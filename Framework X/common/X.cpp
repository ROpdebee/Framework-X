//
//  X.cpp
//  Framework X
//
//  Created by Ruben Opdebeeck on 22/04/2017.
//  Copyright © 2017 Ruben Opdebeeck. All rights reserved.
//

#include "X.hpp"

/// MatchCallback class to allow using AST matchers with RHS templates
class XCallback : public MatchFinder::MatchCallback {
    RHSTemplate &_tmpl;
    std::unique_ptr<Rewriter> _rewriter;
    bool _overwrite;
public:
    XCallback(RHSTemplate &tmpl, bool overwrite) : _tmpl(tmpl), _rewriter(nullptr), _overwrite(overwrite) {}
    
    /// Assign a new rewriter to this callback
    void setRewriter(std::unique_ptr<Rewriter> newRewriter) {
        _rewriter = std::move(newRewriter);
    }
    
    /// Called whenever the current file is fully processed
    void fileProcessed(FileID fid, std::string filename) {
        
        // Replace the file extension with ".transformed.cpp" (or "cc" or any other, depending on the original extension)
        // when we shouldn't overwrite the source files
        if (!_overwrite) {
            // Transform the filename to a vector, as LLVM's replace_extension only accepts vectors
            llvm::SmallVector<char, 128> filenameVector;
            llvm::raw_svector_ostream filenameStream(filenameVector);
            filenameStream << filename; // No need for flushing the buffer, raw_svector_ostream is not buffered
            llvm::sys::path::replace_extension(filenameVector, "transformed" + llvm::sys::path::extension(filename));
            filename = filenameStream.str();
        }
        
        // .write() method of edit buffer only accepts LLVM's raw_ostream,
        // so create a normal output file stream and encapsulate that in an llvm::raw_ostream
        std::ofstream outFile(filename);
        llvm::raw_os_ostream llvmStream(outFile);
        _rewriter->getEditBuffer(fid).write(llvmStream);
    }
    
    void run(const MatchFinder::MatchResult& res) {
        // Use underlying node map to assure we can handle multiple types of nodes
        auto nodes(res.Nodes.getMap());
        auto node(nodes.find("root"));
        assert(node != nodes.end() && "Root node is required");
        
        _rewriter->ReplaceText(node->second.getSourceRange(), _tmpl.instantiate(res));
    }
};

/*/// \class MatcherConsumer
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
};*/

using ASTList = std::vector<std::unique_ptr<ASTUnit>>;

/// \brief Parse the given source files into ASTs, according to the compilation database. Parsed ASTs are inserted into the last parameter.
///
/// Uses a ClangTool to easily parse ASTs, so we do not have to worry about parsing command line options from the compilation database.
/// \param sourceFiles The paths of the source files we need to parse.
/// \param compilations The compilation database
/// \param[out] ASTs The list of generated ASTs.
static void buildASTs(std::vector<std::string> &sourceFiles, const CompilationDatabase &compilations, ASTList &ASTs) {
    ClangTool ASTParserTool(compilations, sourceFiles);
    ASTParserTool.buildASTs(ASTs);
}

/// \brief Consume the ASTs using the given consumer. Will assign a new rewriter to the callback for each file,
/// and notify the callback when the file is completed.
///
/// The callback must be given a new rewriter on each new source file, as the rewriter cannot seem to handle multiple files too well.
/// The consumer will be notified using its `HandleTranslationUnit` method.
/// \param ASTs The list of ASTs to consume. Will be matched in the order presented.
/// \param consumer The AST consumer which does the matching on this AST.
static void consumeASTs(ASTList &ASTs, std::unique_ptr<ASTConsumer> consumer, XCallback &cb) {
    for (auto &AST : ASTs) {
        cb.setRewriter(llvm::make_unique<Rewriter>(AST->getSourceManager(), AST->getLangOpts()));
        consumer->HandleTranslationUnit(AST->getASTContext());
        cb.fileProcessed(AST->getSourceManager().getMainFileID(), AST->getMainFileName());
    }
}

// Many types of Matchers, so use a template to support them all
template <typename MatcherType>
void X::transform(std::vector<std::string> sourceFiles, const CompilationDatabase &compilations, MatcherType matcher,
                  std::string rhs, bool overwriteChangedFiles) {
    
    // Parse the source files to ASTs using a ClangTool
    ASTList ASTs;
    buildASTs(sourceFiles, compilations, ASTs);
    
    // Set up the matching
    RHSTemplate rhsTemplate(rhs);
    MatchFinder finder;
    Rewriter rewriter;
    XCallback cb(rhsTemplate, overwriteChangedFiles);
    finder.addMatcher(matcher, &cb);
    
    // Match the ASTs
    consumeASTs(ASTs, finder.newASTConsumer(), cb);
}


// Explicit initialization of templates so we can still split header and source files
template void X::transform<StatementMatcher>(std::vector<std::string> sourceFiles, const CompilationDatabase &compilations, StatementMatcher matcher, std::string rhs, bool overwriteChangedFiles);
template void X::transform<DeclarationMatcher>(std::vector<std::string> sourceFiles, const CompilationDatabase &compilations, DeclarationMatcher matcher, std::string rhs, bool overwriteChangedFiles);
template void X::transform<TypeMatcher>(std::vector<std::string> sourceFiles, const CompilationDatabase &compilations, TypeMatcher matcher, std::string rhs, bool overwriteChangedFiles);
template void X::transform<TypeLocMatcher>(std::vector<std::string> sourceFiles, const CompilationDatabase &compilations, TypeLocMatcher matcher, std::string rhs, bool overwriteChangedFiles);
template void X::transform<NestedNameSpecifierMatcher>(std::vector<std::string> sourceFiles, const CompilationDatabase &compilations, NestedNameSpecifierMatcher matcher, std::string rhs, bool overwriteChangedFiles);
template void X::transform<NestedNameSpecifierLocMatcher>(std::vector<std::string> sourceFiles, const CompilationDatabase &compilations, NestedNameSpecifierLocMatcher matcher, std::string rhs, bool overwriteChangedFiles);
template void X::transform<CXXCtorInitializerMatcher>(std::vector<std::string> sourceFiles, const CompilationDatabase &compilations, CXXCtorInitializerMatcher matcher, std::string rhs, bool overwriteChangedFiles);
