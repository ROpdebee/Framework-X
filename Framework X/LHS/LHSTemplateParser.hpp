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
#include <memory>

#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTTypeTraits.h>

#include "LHSConfiguration.hpp"

using namespace clang;
using namespace clang::ast_type_traits;
using namespace std;

namespace X {
    
/// \class LHSParserVisitor
/// \brief An LHS template parser implementing the RecursiveASTVisitor class in order to visit all AST nodes of the template source
class LHSParserVisitor : public RecursiveASTVisitor<LHSParserVisitor> {
public:
    LHSParserVisitor(ASTContext &ctx, LHSConfiguration &cfg) : _ctx(ctx), _sm(ctx.getSourceManager()), _cfg(cfg),
        _templateSourceRange(cfg.getTemplateRange()) {}
    
    // Override the Traverse* methods for the base AST nodes
    // Use Traverse* instead of Visit* so we can control the AST traversal,
    // there is no need to descend into AST nodes we know are not part of the template
    virtual bool TraverseStmt(Stmt *S);
    virtual bool TraverseDecl(Decl *D);
    // We don't traverse into types as we don't allow them to be parameterized in templates
    // A correct way to handle type parameterization in templates would be using directives
    // They CAN be included in templates, they get matched within a declaration
    
private:
    ASTContext &_ctx; ///< AST context of the current translation unit
    SourceManager &_sm;
    LHSConfiguration &_cfg;
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
    queue<DynTypedNode> templateSubtrees;
    
    /// \brief Method to generally match a subtree (being either a Stmt or a Decl) to a source range.
    /// \return Boolean indicating if the passed subtree should be traversed further down
    template <class DeclOrStmt>
    bool matchSubtreeToRange(DeclOrStmt *subtree, const TemplateRange &range);
    
    /// \brief Continue traversal on a subtree
    /// \return The result of the traversal
    template <class DeclOrStmt>
    bool continueTraversal(DeclOrStmt *subtree);
    
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
        visitor.TraverseDecl(ctx.getTranslationUnitDecl());
        
        while (!visitor.templateSubtrees.empty()) {
            visitor.templateSubtrees.front().dump(llvm::outs(), sm);
            visitor.templateSubtrees.pop();
        }
    }
};

} // namespace X

#endif /* LHSTemplateParser_hpp */
