//
//  LHSTemplate.hpp
//  Framework X
//
//  Created by Ruben Opdebeeck on 07/05/2017.
//  Copyright © 2017 Ruben Opdebeeck. All rights reserved.
//

#ifndef LHSTemplate_hpp
#define LHSTemplate_hpp

#include <clang/Frontend/ASTUnit.h>

#include "LHSTemplateParser.hpp"
#include "ASTTraversalState.hpp"
#include "LHSComparators.hpp"

using namespace std;
using namespace X;
using namespace clang::ast_type_traits;
using namespace clang;

namespace X {

/// One match, containing a list of root nodes for the AST
/// and a map for all bound metavariables.
struct MatchResult {
    vector<DynTypedNode> root;
    map<Metavariable, ASTNode> metavariables;
    MatchResult(vector<DynTypedNode> roots, map<Metavariable, ASTNode> metas) : root(roots), metavariables(metas) {}
};

/// A list of matches for a certain AST.
struct ASTResult {
    shared_ptr<ASTUnit> ast;
    vector<MatchResult> matches;
    ASTResult(shared_ptr<ASTUnit> astUnit, vector<MatchResult> res) : ast(astUnit), matches(res) {}
};

/// \class LHSTemplate
/// \brief Represents a LHS template
class LHSTemplate {
    vector<DynTypedNode> _templateSubtrees;
    
    /// Keep a mapping from subtrees (Stmts and Decls) of the main template to the metavariables
    /// We cannot create new AST nodes easily, nor can we alter them, so there is no way for us
    /// to embed metavariable information directly in the template AST. Hence, when comparing
    /// ASTs to the template, we must check if the AST node in the template is in this map to
    /// determine whether or not it is a metavariable.
    /// Note that a metavariable may be mapped to multiple times if it represents multiple subtrees,
    /// this is fine because the template doesn't need to know what is underneath the metavariable.
    map<DynTypedNode, Metavariable> _metavariables;
    
public:
    LHSTemplate() {}
    
    /// Append an AST subtree to the end of the template subtree list
    void addTemplateSubtree(DynTypedNode subtree);
    
    /// Add a metavariable to the template
    /// \param meta The metavariable
    /// \param subtree The subtree this metavariable corresponds to in the main template
    void addMetavariable(Metavariable meta, DynTypedNode subtree);
    
    /// Determine whether or not a subtree is represented by a metavariable in the template
    bool isMetavariable(DynTypedNode subtree);
    
    /// Retrieve the metavariable representing this subtree
    Metavariable getMetavariable(DynTypedNode subtree);
    
    /// Match the LHS template on a list of ASTs
    /// It returns a list of match results. The results are ordered
    /// based on the source ranges of the matches, with matches that
    /// start earlier in the source file occurring earlier in the
    /// result list. It is guaranteed that match results do not overlap.
    /// If the matching algorithm found two overlapping matches,
    /// only the larger of the two matches (the one whose root lies
    /// higher in the AST) is included in the returned list.
    /// This is to prevent corrupting the transformed source files
    /// when overlapping regions are rewritten.
    vector<ASTResult> matchAST(vector<shared_ptr<ASTUnit>> asts);
    
    /// Dump the template. Used for debugging purposes
    void dump(SourceManager &sm);
};

} // namespace X

#endif /* LHSTemplate_hpp */
