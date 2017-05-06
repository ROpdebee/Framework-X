//
//  X.cpp
//  Framework X
//
//  Created by Ruben Opdebeeck on 22/04/2017.
//  Copyright © 2017 Ruben Opdebeeck. All rights reserved.
//

#include "X.hpp"

class InternalCallback : public XCallback {
    RHSTemplate &_tmpl;
    bool _overwrite;
    
public:
    InternalCallback(RHSTemplate &tmpl, bool overwrite) : _tmpl(tmpl), _overwrite(overwrite) {}
    
    // Keep the default setRewriter implementation
    
    void fileProcessed(FileID fid, std::string filename) override {
        
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
        _pRewriter->getEditBuffer(fid).write(llvmStream);
    }
    
    void run(const MatchFinder::MatchResult& res) override {
        // Use underlying node map to assure we can handle multiple types of nodes
        auto nodes(res.Nodes.getMap());
        auto node(nodes.find("root"));
        assert(node != nodes.end() && "Root node is required");
        
        _pRewriter->ReplaceText(node->second.getSourceRange(), _tmpl.instantiate(res));
    }
};

using ASTList = std::vector<std::unique_ptr<ASTUnit>>;

/// \brief Parse the given source files into ASTs, according to the compilation database. Parsed ASTs are inserted into the last parameter.
///
/// Uses a ClangTool to easily parse ASTs, so we do not have to worry about parsing command line options from the compilation database.
/// \param sourceFiles The paths of the source files we need to parse.
/// \param compilations The compilation database
/// \param[out] ASTs The list of generated ASTs.
static void buildASTs(const std::vector<std::string> &sourceFiles, const CompilationDatabase &compilations, ASTList &ASTs) {
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
void X::transform(const std::vector<std::string> &sourceFiles, const CompilationDatabase &compilations, MatcherType &matcher,
                  std::string rhs, bool overwriteChangedFiles) {
    
    // Parse the source files to ASTs using a ClangTool
    ASTList ASTs;
    buildASTs(sourceFiles, compilations, ASTs);
    
    // Set up the matching
    RHSTemplate rhsTemplate(rhs);
    MatchFinder finder;
    InternalCallback cb(rhsTemplate, overwriteChangedFiles);
    finder.addMatcher(matcher, &cb);
    
    // Match the ASTs
    consumeASTs(ASTs, finder.newASTConsumer(), cb);
}

template <typename MatcherType>
void X::transform(const std::vector<std::string> &sourceFiles, const CompilationDatabase &compilations, MatcherType &matcher, XCallback &cb) {
    
    ASTList ASTs;
    buildASTs(sourceFiles, compilations, ASTs);
    MatchFinder finder;
    finder.addMatcher(matcher, &cb);
    
    consumeASTs(ASTs, finder.newASTConsumer(), cb);
}

            
// Explicit initialization of templates so we can still split header and source files
template void X::transform<StatementMatcher>(const std::vector<std::string> &sourceFiles, const CompilationDatabase &compilations, StatementMatcher &matcher, std::string rhs, bool overwriteChangedFiles);
template void X::transform<DeclarationMatcher>(const std::vector<std::string> &sourceFiles, const CompilationDatabase &compilations, DeclarationMatcher &matcher, std::string rhs, bool overwriteChangedFiles);
template void X::transform<TypeMatcher>(const std::vector<std::string> &sourceFiles, const CompilationDatabase &compilations, TypeMatcher &matcher, std::string rhs, bool overwriteChangedFiles);
template void X::transform<TypeLocMatcher>(const std::vector<std::string> &sourceFiles, const CompilationDatabase &compilations, TypeLocMatcher &matcher, std::string rhs, bool overwriteChangedFiles);
template void X::transform<NestedNameSpecifierMatcher>(const std::vector<std::string> &sourceFiles, const CompilationDatabase &compilations, NestedNameSpecifierMatcher &matcher, std::string rhs, bool overwriteChangedFiles);
template void X::transform<NestedNameSpecifierLocMatcher>(const std::vector<std::string> &sourceFiles, const CompilationDatabase &compilations, NestedNameSpecifierLocMatcher &matcher, std::string rhs, bool overwriteChangedFiles);
template void X::transform<CXXCtorInitializerMatcher>(const std::vector<std::string> &sourceFiles, const CompilationDatabase &compilations, CXXCtorInitializerMatcher &matcher, std::string rhs, bool overwriteChangedFiles);

// Same for LHS matchers, RHS callback version of transform
template void X::transform<StatementMatcher>(const std::vector<std::string> &sourceFiles, const CompilationDatabase &compilations, StatementMatcher &matcher, XCallback &cb);
template void X::transform<DeclarationMatcher>(const std::vector<std::string> &sourceFiles, const CompilationDatabase &compilations, DeclarationMatcher &matcher, XCallback &cb);
template void X::transform<TypeMatcher>(const std::vector<std::string> &sourceFiles, const CompilationDatabase &compilations, TypeMatcher &matcher, XCallback &cb);
template void X::transform<TypeLocMatcher>(const std::vector<std::string> &sourceFiles, const CompilationDatabase &compilations, TypeLocMatcher &matcher, XCallback &cb);
template void X::transform<NestedNameSpecifierMatcher>(const std::vector<std::string> &sourceFiles, const CompilationDatabase &compilations, NestedNameSpecifierMatcher &matcher, XCallback &cb);
template void X::transform<NestedNameSpecifierLocMatcher>(const std::vector<std::string> &sourceFiles, const CompilationDatabase &compilations, NestedNameSpecifierLocMatcher &matcher, XCallback &cb);
template void X::transform<CXXCtorInitializerMatcher>(const std::vector<std::string> &sourceFiles, const CompilationDatabase &compilations, CXXCtorInitializerMatcher &matcher, XCallback &cb);
