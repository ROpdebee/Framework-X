//
//  X.cpp
//  Framework X
//
//  Created by Ruben Opdebeeck on 22/04/2017.
//  Copyright © 2017 Ruben Opdebeeck. All rights reserved.
//

#include "X.hpp"

/// Static instance of framework X
static XInstance xi;

/// MatchCallback class to allow using AST matchers with RHS templates
class XCallback : public MatchFinder::MatchCallback {
    RHSTemplate &_tmpl;
    Rewriter &_rewriter;
public:
    XCallback(RHSTemplate &tmpl, Rewriter &rewriter) : _tmpl(tmpl), _rewriter(rewriter) {}
    
    void run(const MatchFinder::MatchResult& res) {
        // Use underlying node map to assure we can handle multiple types of nodes
        auto nodes(res.Nodes.getMap());
        auto node(nodes.find("root"));
        assert(node != nodes.end() && "Root node is required");
        
        _rewriter.ReplaceText(node->second.getSourceRange(), _tmpl.instantiate(res));
    }
};

/// \class MatcherConsumer
/// \brief Custom class to allow us to avoid using Replacements and a RefactoringTool while still using AST matchers
template <typename MatcherType>
class MatcherConsumer : public ASTConsumer {
    MatchFinder _finder;
    XCallback _cb;
public:
    MatcherConsumer(MatcherType matcher, RHSTemplate &tmpl, Rewriter &rewriter) : _cb(tmpl, rewriter) {
        _finder.addMatcher(matcher, &_cb);
    }
    
    void HandleTranslationUnit(ASTContext &ctx) override {
        _finder.matchAST(ctx);
    }
};

/// \class TransformationAction
/// \brief An action that gets created for each source file and can create AST consumers for this file
template <typename MatcherType>
class TransformationAction : public ASTFrontendAction {
    MatcherType _matcher;
    RHSTemplate &_tmpl;
    Rewriter _rewriter;
public:
    TransformationAction(MatcherType matcher, RHSTemplate &tmpl) : _matcher(matcher), _tmpl(tmpl) {}
    
    /// Flush rewritten changes to disk
    void EndSourceFileAction() override {
        if (_rewriter.overwriteChangedFiles()) llvm::outs() << "Failed to save a file!\n";
    }
    
    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file) override {
        _rewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
        return llvm::make_unique<MatcherConsumer<MatcherType>>(_matcher, _tmpl, _rewriter);
    }
};

/// \class TransformationActionFactory
/// \brief Factory class to create TransformationActions
/// \see FrontendActionFactory
/// We need to make our own factory so that we can pass the necessary arguments to the AST consumer
template <typename MatcherType>
class TransformationActionFactory : public FrontendActionFactory {
    MatcherType _matcher;
    RHSTemplate &_tmpl;
public:
    TransformationActionFactory(MatcherType matcher, RHSTemplate &tmpl) : _matcher(matcher), _tmpl(tmpl) {}
    
    TransformationAction<MatcherType> *create() override {
        return new TransformationAction<MatcherType>(_matcher, _tmpl);
    }
};

// Many types of Matchers, so use a template to support them all
template <typename MatcherType>
void X::transform(std::vector<std::string> sourceFiles, const CompilationDatabase &compilations, MatcherType matcher, std::string rhs) {
    RHSTemplate rhsTemplate(rhs, xi);
    
    ClangTool tool(compilations, sourceFiles);
    
    // Invoke the tool with a new action factory
    tool.run(llvm::make_unique<TransformationActionFactory<MatcherType>>(matcher, rhsTemplate).get());
}


// Explicit initialization of templates so we can still split header and source files
template void X::transform<StatementMatcher>(std::vector<std::string> sourceFiles, const CompilationDatabase &compilations, StatementMatcher matcher, std::string rhs);
template void X::transform<DeclarationMatcher>(std::vector<std::string> sourceFiles, const CompilationDatabase &compilations, DeclarationMatcher matcher, std::string rhs);
template void X::transform<TypeMatcher>(std::vector<std::string> sourceFiles, const CompilationDatabase &compilations, TypeMatcher matcher, std::string rhs);
template void X::transform<TypeLocMatcher>(std::vector<std::string> sourceFiles, const CompilationDatabase &compilations, TypeLocMatcher matcher, std::string rhs);
template void X::transform<NestedNameSpecifierMatcher>(std::vector<std::string> sourceFiles, const CompilationDatabase &compilations, NestedNameSpecifierMatcher matcher, std::string rhs);
template void X::transform<NestedNameSpecifierLocMatcher>(std::vector<std::string> sourceFiles, const CompilationDatabase &compilations, NestedNameSpecifierLocMatcher matcher, std::string rhs);
template void X::transform<CXXCtorInitializerMatcher>(std::vector<std::string> sourceFiles, const CompilationDatabase &compilations, CXXCtorInitializerMatcher matcher, std::string rhs);
