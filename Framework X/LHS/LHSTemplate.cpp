//
//  LHSTemplate.cpp
//  Framework X
//
//  Created by Ruben Opdebeeck on 07/05/2017.
//  Copyright © 2017 Ruben Opdebeeck. All rights reserved.
//

#include "LHSTemplate.hpp"

void LHSTemplate::addTemplateSubtree(DynTypedNode subtree) {
    _templateSubtrees.push_back(subtree);
}

void LHSTemplate::addMetavariable(Metavariable meta, DynTypedNode subtree) {
    auto it = _metavariables.find(subtree);
    if (it != _metavariables.end()) {
        it->second = meta;
    } else {
        _metavariables.insert({ subtree, meta });
    }
    
    // When a class declaration is parameterized with a name-only metavariable, we need to
    // make sure its constructors and destructors get turned into name-only metavariables as well
    if (const CXXRecordDecl *record = subtree.get<CXXRecordDecl>()) {
        Metavariable implicitNameOnlyMeta("__implicit_metavariable");
        implicitNameOnlyMeta.nameOnly = true;
        
        bool parameterizeThisDecl = false;
        for (const auto innerDecl : record->decls()) {
            switch (innerDecl->getKind()) {
                case clang::Decl::CXXConversion:
                case clang::Decl::CXXDestructor:
                case clang::Decl::CXXConstructor:
                parameterizeThisDecl = true;
                break;
                
                case clang::Decl::CXXRecord: {
                    if (cast<CXXRecordDecl>(innerDecl)->isInjectedClassName()) {
                        parameterizeThisDecl = true;
                    }
                    break;
                }
                
                default: break;
            }
            
            if (parameterizeThisDecl) {
                DynTypedNode dtn = DynTypedNode::create(*innerDecl);
                _metavariables.insert({ dtn, implicitNameOnlyMeta });
                parameterizeThisDecl = false;
            }
        }
    }
}

bool LHSTemplate::isMetavariable(DynTypedNode subtree) {
    return _metavariables.find(subtree) != _metavariables.end();
}

Metavariable LHSTemplate::getMetavariable(DynTypedNode subtree) {
    auto it = _metavariables.find(subtree);
    assert(it != _metavariables.end() && "Not a metavariable");
    return it->second;
}

/// \class PotentialMatchFinder
/// \brief Helper class whose goal is to find the first potential matches.
/// Using a recursive AST visitor, it will visit each and every Decl and Stmt
/// and find AST nodes whose class (type) match the first AST node in our template.
/// It then creates a potential match whose root contains all siblings following the
/// found node. As our template can potentially span multiple AST subtrees, all
/// of the following siblings must be added in order to allow a full match.
/// Since one of the template subtrees may be a metaparameter, it is vital that
/// all of the siblings are added, as a metaparameter can potentially match tens of
/// subtrees.
class PotentialMatchFinder : public RecursiveASTVisitor<PotentialMatchFinder> {
    DynTypedNode lhsRoot;
    shared_ptr<ASTUnit> astUnit;
    ASTContext &ctx;
    SourceManager &sm;
    vector<PotentialMatch> &potentialMatches;
    
    public:
    PotentialMatchFinder(DynTypedNode root, shared_ptr<ASTUnit> ast, vector<PotentialMatch> &potentials)
        : lhsRoot(root), astUnit(ast), ctx(ast->getASTContext()), sm(ast->getSourceManager()), potentialMatches(potentials) {}
    
    bool VisitStmt(Stmt *S) {
        // Ignore header Stmts
        if (!sm.isWrittenInMainFile(S->getLocStart())) return true;
        
        if (lhsRoot.getNodeKind().isSame(ASTNodeKind::getFromNode(*S))) {
            // Potential match found
            // Find parent, create ASTNode with parent and child, create PotentialMatch
            DynTypedNode child(DynTypedNode::create(*S));
            auto parent(ctx.getParents(child)[0]);
            vector<ASTNode> potentials(ASTNode::fromParentAndChild(parent, child));
            for (auto &pot : potentials) {
                potentialMatches.push_back({ pot, astUnit });
            }
        }
        
        return true;
    }
    
    bool VisitDecl(Decl *D) {
        // Ignore header Decls
        if (!sm.isWrittenInMainFile(D->getLocStart())) return true;
        
        if (lhsRoot.getNodeKind().isSame(ASTNodeKind::getFromNode(*D))) {
            // Potential match found
            // Find parent, create ASTNode with parent and child, create PotentialMatch
            DynTypedNode child(DynTypedNode::create(*D));
            auto parent(ctx.getParents(child)[0]);
            vector<ASTNode> potentials(ASTNode::fromParentAndChild(parent, child));
            for (auto &pot : potentials) {
                potentialMatches.push_back({ pot, astUnit });
            }
        }
        
        return true;
    }
};

// Static lambda's to check certain conditions in an AST traversal
static auto backtrackedFromChild = [](ASTTraversalState &trv) { return trv.childrenAccessed(); };
static auto lastChild = [](ASTTraversalState &trv) { return trv.isLastChild(); };
static auto notLastChild = [](ASTTraversalState &trv) { return !trv.isLastChild(); };
static auto backtrackToParent = [](ASTTraversalState &trv) { trv.backtrackToParent(); };
static auto proceedToSibling = [](ASTTraversalState &trv) { trv.nextSibling(); };
static auto hasChildren = [](ASTTraversalState &trv) { return trv.hasChildren(); };
static auto descendToChildren = [](ASTTraversalState &trv) { trv.descendToChild(); };
static auto childlessLastNode = [](ASTTraversalState &trv) { return trv.isLastChild() && !trv.hasChildren(); };
static auto childlessWithSibling = [](ASTTraversalState &trv) { return !trv.isLastChild() && !trv.hasChildren(); };

// Simple function to remove elements from the potential matches vector if they don't pass a predicate.
// When predicate returns true for an element, it is kept. Otherwise, it is removed.
static void filter(vector<PotentialMatch> &potentialMatches, function<bool(ASTTraversalState &)> predicate) {
    potentialMatches.erase(remove_if(potentialMatches.begin(), potentialMatches.end(), not1(predicate)),
                           potentialMatches.end());
}

// Simple function to perform an action on all elements of the potential matches vector
static void foreach(vector<PotentialMatch> &potentialMatches, function<void(PotentialMatch &)> action) {
    for (PotentialMatch &pot : potentialMatches) {
        action(pot);
    }
}

vector<ASTResult> LHSTemplate::matchAST(vector<shared_ptr<ASTUnit>> asts) {
    // First gather all potential matches via a RecursiveASTVisitor
    vector<PotentialMatch> potentialMatches;
    for (auto ast : asts) {
        PotentialMatchFinder pmf(_templateSubtrees[0], ast, potentialMatches);
        pmf.TraverseDecl(ast->getASTContext().getTranslationUnitDecl());
    }
    
    // Then start the actual matching on the ASTs.
    vector<ASTNode> lhsSubtrees;
    for (auto &subtree : _templateSubtrees) {
        lhsSubtrees.push_back(ASTNode(subtree));
    }
    ASTNode lhsRoot(lhsSubtrees);
    ASTTraversalState templateTraversal(lhsRoot);
    
    while (!templateTraversal.astProcessed()) {
        DynTypedNode &curr(templateTraversal.getCurrent());
        
        // There are two cases: either we have backtracked from a child, in which case
        // the children will be processed, or we came from a sibling or descended from a parent.
        // In the former case, we need to either proceed to our next sibling, or backtrack to the
        // parent if we're the last child. No comparison should be done, as that is already done in
        // the previous step. In the latter case, we need to compare nodes, descend into children if
        // there are any, or proceed to the next sibling if we have no children.
        
        // If we have backtracked from a child, either continue to the next sibling or backtrack to
        // our parent, if we're the last child. Verify and adjust the traversal in the potential matches
        // in either case.
        if (backtrackedFromChild(templateTraversal)) {
            if (lastChild(templateTraversal)) {
                // Remove any potential match that is not at the last child
                filter(potentialMatches, lastChild);
                foreach(potentialMatches, backtrackToParent);
                templateTraversal.backtrackToParent();
            } else {
                // Proceed to sibling, keep only potential matches that are not the last child
                filter(potentialMatches, notLastChild);
                foreach(potentialMatches, proceedToSibling);
                templateTraversal.nextSibling();
            }
        }
        
        // When we come from a parent or a previous sibling, first we need to compare the node itself.
        // Afterwards, when there are children, descend into them, otherwise, go to the next sibling.
        // An exception to this rule is when the current node is a metaparameter, then we don't need to
        // compare anything and just instantiate the metaparameter
        else {
            if (isMetavariable(curr)) {
                Metavariable meta(getMetavariable(curr));
                
                // For metavariables which only parameterize the name, we still need to match everything else
                if (meta.nameOnly) {
                    // Match the nodes, except their names
                    filter(potentialMatches, [&curr](auto &pot) { return compare(curr, pot.getCurrent(), true); });
                    // For each remaining potential match, take the current node as the instantiation of the metavariable
                    // Name-only metavariables can only span one node, one NamedDecl, so there is no need for extending
                    foreach(potentialMatches, [&meta](auto &pot) { pot.instantiateCurrentAsMetavariable(meta); });
                    
                    // We still need to match the children of this node, to make sure the name-only metavar also matches these
                    // This is done further down
                } else {
                    // A fully parameterized template metavariable may instantiate multiple AST nodes, hence our potential match
                    // list should be extended with copies of the existing potential matches, each instantiating a part of our
                    // siblings as the metavariable. Inconsistent instantiations will be removed later on. There is no need to
                    // traverse our children.
                    vector<PotentialMatch> extendedList;
                    foreach(potentialMatches, [&meta, &extendedList](auto &pot) { pot.extendForMetavariable(meta, extendedList); });
                    potentialMatches = extendedList;
                    
                    // Traverse to the next sibling if there is any, otherwise go back to the parent. Remove inconsistencies
                    // We don't care about children as we've just instantiated a fully parameterized metavariable.
                    if (templateTraversal.isLastChild()) {
                        filter(potentialMatches, lastChild);
                        foreach(potentialMatches, backtrackToParent);
                        templateTraversal.backtrackToParent();
                    } else {
                        filter(potentialMatches, notLastChild);
                        foreach(potentialMatches, proceedToSibling);
                        
                        // A metavariable can span multiple nodes in the AST, so proceed to the next sibling that is not part
                        // of this metavariable
                        templateTraversal.nextSibling();
                        for (DynTypedNode *next = &templateTraversal.getCurrent();
                             isMetavariable(*next) && getMetavariable(*next).identifier == meta.identifier;
                             next = &templateTraversal.nextSibling()) {
                            if (templateTraversal.isLastChild()) {
                                filter(potentialMatches, lastChild);
                                foreach(potentialMatches, backtrackToParent);
                                templateTraversal.backtrackToParent();
                                break;
                            }
                        }
                    }
                    
                    continue; // No need to do the rest of the checks anymore
                }
            } else {
                // For nodes that are not parameterized, we need to compare the AST nodes and remove inconsistenties.
                filter(potentialMatches, [&curr](auto &pot) { return compare(curr, pot.getCurrent()); });
            }
            
            // If there are children, descend to them if we need to
            // Remove potential matches without children
            if (templateTraversal.hasChildren()) {
                filter(potentialMatches, hasChildren);
                foreach(potentialMatches, descendToChildren);
                templateTraversal.descendToChild();
            }
            
            // Otherwise, proceed to the next sibling if there is one.
            else if (templateTraversal.isLastChild()) {
                filter(potentialMatches, childlessLastNode);
                foreach(potentialMatches, backtrackToParent);
                templateTraversal.backtrackToParent();
            } else {
                filter(potentialMatches, childlessWithSibling);
                foreach(potentialMatches, proceedToSibling);
                templateTraversal.nextSibling();
            }
        }
    }
    
    // Partition the match results into separate lists for each file given to us
    vector<ASTResult> resultsForFiles;
    for (auto &ast : asts) {
        SourceManager &sm(ast->getSourceManager());
        vector<unique_ptr<pair<MatchResult, TemplateRange>>> resultRanges;
        
        // Gather all match results, together with their ranges, for this AST
        foreach(potentialMatches, [&](auto &pot) {
            auto roots = pot.getMatchRoot();
            SourceLocation begin(roots[0].getSourceRange().getBegin());
            SourceLocation end(roots[roots.size()-1].getSourceRange().getEnd());
            if (ast == pot.getOwner()) {
                TemplateRange tr(TemplateLocation::fromSourceLocation(begin, sm),
                                 TemplateLocation::fromSourceLocation(end, sm));
                MatchResult mr(roots, pot.getMetavariables());
                resultRanges.push_back(llvm::make_unique<pair<MatchResult, TemplateRange>>(mr, tr));
            }
        });
        
        if (resultRanges.empty()) continue;
        
        // Sort the results based on their ranges
        sort(resultRanges.begin(), resultRanges.end(), [](auto &p, auto &q) { return p->second < q->second; });
        
        // Eliminate overlapping results, keep the result that occurs first in the source code
        vector<MatchResult> results;
        results.push_back(resultRanges[0]->first);
        for (unsigned i = 1; i < resultRanges.size(); i++) {
            if (!resultRanges[i-1]->second.overlapsWith(resultRanges[i]->second)) {
                results.push_back(resultRanges[i]->first);
            } else {
                cerr << "Removing a potential match as it overlaps with another one\n\tFile: " << ast->getMainFileName().str()
                << "\n\tSource ranges: " << resultRanges[i-1]->second << " and " << resultRanges[i]->second << "\n";
            }
        }
        
        resultsForFiles.push_back(ASTResult(ast, results));
    }
    
    return resultsForFiles;
}

void LHSTemplate::dump(SourceManager &sm) {
    llvm::outs() << "Template subtrees:\n~~~~~~~~~~~~~~~~~~\n\n";
    for (auto &subtree : _templateSubtrees) {
        subtree.dump(llvm::outs(), sm);
        llvm::outs() << "\n\n";
    }
    
    llvm::outs() << "Metavariables:\n~~~~~~~~~~~~~~\n\n";
    for (auto &metaPair : _metavariables) {
        llvm::outs() << metaPair.second.identifier;
        if (metaPair.second.nameOnly) {
            llvm::outs() << " [name-only]";
        }
        llvm::outs() << ":\n";
        metaPair.first.dump(llvm::outs(), sm);
        llvm::outs() << "\n\n";
    }
    
}
