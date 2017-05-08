//
//  LHSTemplateParser.hpp
//  Framework X
//
//  Created by Ruben Opdebeeck on 07/05/2017.
//  Copyright © 2017 Ruben Opdebeeck. All rights reserved.
//

#ifndef LHSTemplateParser_hpp
#define LHSTemplateParser_hpp

#include <queue>
#include <vector>
#include <map>
#include <set>
#include <memory>

#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTTypeTraits.h>

#include "LHSConfiguration.hpp"

using namespace clang;
using namespace clang::ast_type_traits;
using namespace std;

namespace X {

/// \class StmtOrDecl
/// \brief A small encapsulation class that can contain both Stmts and Decls
/// The motivation behind this is that we need some way to support and save multiple
/// node kinds. DynTypedNode does not work for us in this case, as it returns its
/// storage as a const value and this doesn't get accepted by the visitor (it needs
/// non-const nodes). Instead of using const_cast (which may result in undefined behaviour),
/// we roll our own generic container.
class StmtOrDecl {
public:
    enum Kind { STMT, DECL };
    
    StmtOrDecl(Stmt *stmt) : _kind(STMT) { _node = stmt; }
    StmtOrDecl(Decl *decl) : _kind(DECL) { _node = decl; }
    StmtOrDecl() {}
    StmtOrDecl(StmtOrDecl const &other) : _kind(other._kind) { _node = other._node; }
    
    inline Kind getKind() const { return _kind; }
    
    inline Stmt *getAsStmt() const {
        if (_kind == STMT) return static_cast<Stmt *>(_node);
        else throw runtime_error("Retrieving a Decl as a Stmt");
    }
    
    inline Decl *getAsDecl() const {
        if (_kind == DECL) return static_cast<Decl *>(_node);
        else throw runtime_error("Retrieving a Stmt as a Decl");
    }
    
    inline SourceLocation getLocStart() {
        if (_kind == DECL) return getAsDecl()->getLocStart();
        else return getAsStmt()->getLocStart();
    }
    inline SourceLocation getLocEnd() {
        if (_kind == DECL) return getAsDecl()->getLocEnd();
        else return getAsStmt()->getLocEnd(); }
    
private:
    void *_node; ///< The Stmt or Decl node
    Kind _kind; ///< The kind of node contained
};
    
using SubtreeList = vector<StmtOrDecl>;
using SubtreeQueue = queue<StmtOrDecl>;
    

/// \class LHSParserVisitor
/// \brief An LHS template parser implementing the RecursiveASTVisitor class in order to visit all AST nodes of the template source
class LHSParserVisitor : public RecursiveASTVisitor<LHSParserVisitor> {
public:
    LHSParserVisitor(ASTContext &ctx, LHSConfiguration &cfg) : _sm(ctx.getSourceManager()), _templateSourceRange(cfg.getTemplateRange()) {
        auto metavars(cfg.getMetavariableRanges());
        remainingMetavariables = set<MetavarLoc>(metavars.begin(), metavars.end());
    }
    
    // Override the Traverse* methods for the base AST nodes
    // Use Traverse* instead of Visit* so we can control the AST traversal,
    // there is no need to descend into AST nodes we know are not part of the template
    virtual bool TraverseStmt(Stmt *S);
    virtual bool TraverseDecl(Decl *D);
    // We don't traverse into types as we don't allow them to be parameterized in templates
    // A correct way to handle type parameterization in templates would be using directives
    // They CAN be included in templates, they get matched within a declaration
    
private:
    SourceManager &_sm;
    const TemplateRange &_templateSourceRange;
    
    //
    // State variables to indicate the progress of parsing the template
    //
    
    /// When true, the main template has been fully found
    /// and the metavariables should be parsed
    bool templateParsed = false;
    
    /// When true, a previous AST node has started template
    /// construction, but the full template spans multiple subtrees
    /// so we should parse further until we completed the template
    bool templateConstructionBegan = false;
    
    /// A queue of subtrees that need to be searched for template metavariables
    SubtreeQueue templateSubtrees;
    
    /// The set of metavariables that still need to be parsed
    set<MetavarLoc> remainingMetavariables;
    
    /// A mapping containing the metavariables that have been parsed
    /// It maps from a metavariable identifier to a sequence of subtrees
    /// that the metavariable represents
    map<string, SubtreeList> parsedMetavariables;
    
    /// \brief Method to generally parse a subtree (being either a Stmt or a Decl) to a template.
    /// \return Boolean indicating if the the template was parsed
    bool parseSubtreeToTemplate(StmtOrDecl subtree);
    
    /// \brief Method to parse metavariables in a subtree
    /// \return Boolean indicating if the traversal should continue
    bool parseMetavariables(StmtOrDecl subtree);
    
    /// The current metavariable being parsed, or nullptr when no metavariable is being parsed
    /// If this is set when entering a subtree, the subtree is being inspected to add to the sequence
    /// of subtrees for this metavariable
    const MetavarLoc *parsingMetavariable = nullptr;
    
    /// \brief Continue traversal on a subtree
    /// \return The result of the traversal
    bool continueTraversal(StmtOrDecl subtree);
    
    ~LHSParserVisitor() {} // Suppress warnings about non-virtual destructor
    
    friend class LHSParserConsumer;
};
    
/// \class LHSParserConsumer
/// \brief An AST consumer that forwards translation units to the LHS template parser
class LHSParserConsumer : public ASTConsumer {
    LHSConfiguration &_cfg;
    
public:
    LHSParserConsumer(LHSConfiguration &cfg) : _cfg(cfg) {}
    void HandleTranslationUnit(ASTContext &ctx) {
        
        // First make sure the template can actually be found in the AST
        // When the template range is larger than the source file itself, it will never be correct
        SourceManager &sm(ctx.getSourceManager());
        TemplateLocation sof(TemplateLocation::fromSourceLocation(sm.getLocForStartOfFile(sm.getMainFileID()), sm));
        TemplateLocation eof(TemplateLocation::fromSourceLocation(sm.getLocForEndOfFile(sm.getMainFileID()), sm));
        TemplateRange fileRange(sof, eof);
        if (!_cfg.getTemplateRange().enclosedIn(fileRange)) {
            throw MalformedConfigException("Template range is larger than source file range");
        }
        
        // Make a new visitor for each translation unit, as it needs the new context
        LHSParserVisitor visitor(ctx, _cfg);
        
        // This will parse the template
        visitor.TraverseDecl(ctx.getTranslationUnitDecl());
        
        // When we're done parsing the template, continue with parsing the metavariables
        if (!visitor.templateParsed) {
            throw MalformedConfigException("No template was parsed");
        }
        
        // Traverse each subtree in the template for metavariables
        StmtOrDecl subtree;
        while (!visitor.templateSubtrees.empty()) {
            subtree = visitor.templateSubtrees.front();
            
            if (subtree.getKind() == StmtOrDecl::Kind::DECL) {
                subtree.getAsDecl()->dump();
                visitor.TraverseDecl(subtree.getAsDecl());
            } else {
                visitor.TraverseStmt(subtree.getAsStmt());
                subtree.getAsStmt()->dump();
            }
            
            visitor.templateSubtrees.pop();
        }
        
        // Make sure each metavariable is parsed
        if (!visitor.remainingMetavariables.empty()) {
            for (auto &meta : visitor.remainingMetavariables) {
                llvm::errs() << "Failed to parse metavariable " << meta.identifier;
            }
            throw MalformedConfigException("Some metavariables could not be parsed");
        }
        
        for (auto &meta : visitor.parsedMetavariables) {
            llvm::outs() << "Metavariable " << meta.first << "\n";
            for (auto &subtree : meta.second) {
                if (subtree.getKind() == StmtOrDecl::Kind::DECL) {
                    subtree.getAsDecl()->dump();
                } else {
                    subtree.getAsStmt()->dump();
                }
            }
            llvm::outs() << "\n";
        }
    }
};

} // namespace X

#endif /* LHSTemplateParser_hpp */
