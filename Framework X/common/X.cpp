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
    
    void fileProcessed(FileID fid, string filename) override {
        
        // Replace the file extension with ".transformed.cpp" (or "cc" or any other, depending on the original extension)
        // when we shouldn't overwrite the source files
        if (!_overwrite) {
            // Transform the filename to a vector, as LLVM's replace_extension only accepts vectors
            llvm::SmallVector<char, 128> filenameVector;
            llvm::raw_svector_ostream filenameStream(filenameVector);
            // Ugly stream insertion, Xcode seems to think we're starting a template instantiation when using custom stream insertion operators, leading to issues with code indentation...
            filenameStream.operator<<(filename); // No need for flushing the buffer, raw_svector_ostream is not buffered
            llvm::sys::path::replace_extension(filenameVector, "transformed" + llvm::sys::path::extension(filename));
            filename = filenameStream.str();
        }
        
        // .write() method of edit buffer only accepts LLVM's raw_ostream,
        // so create a normal output file stream and encapsulate that in an llvm::raw_ostream
        ofstream outFile(filename);
        llvm::raw_os_ostream llvmStream(outFile);
        _pRewriter->getEditBuffer(fid).write(llvmStream);
    }
    
    void run(const MatchFinder::MatchResult& res) override {
        // Use underlying node map to assure we can handle multiple types of nodes
        auto nodes(res.Nodes.getMap());
        auto node(nodes.find("root"));
        assert(node != nodes.end() && "Root node is required");
        
        SourceRange sr(node->second.getSourceRange());
        SourceManager &sm(_pRewriter->getSourceMgr());
        const LangOptions &lops(_pRewriter->getLangOpts());
        
        // Make sure trailing literals in the root's source range are fully included in the range
        sr.setEnd(X::Lexer::getEndOfLiteral(sr.getEnd(), sm, lops));
        
        // Extend the source range to also include the trailing semicolon, if there is one
        SourceLocation trailingSemiLoc(X::Lexer::getSemiAfterLocation(sr.getEnd(), _pRewriter->getSourceMgr(), _pRewriter->getLangOpts()));
        if (trailingSemiLoc.isValid()) {
            sr.setEnd(trailingSemiLoc);
        }
        
        _pRewriter->ReplaceText(sr, _tmpl.instantiate(res));
    }
};

using ASTList = vector<unique_ptr<ASTUnit>>;

/// \brief Parse the given source files into ASTs, according to the compilation database. Parsed ASTs are inserted into the last parameter.
///
/// Uses a ClangTool to easily parse ASTs, so we do not have to worry about parsing command line options from the compilation database.
/// \param sourceFiles The paths of the source files we need to parse.
/// \param compilations The compilation database
/// \param[out] ASTs The list of generated ASTs.
static void buildASTs(const SourceList &sourceFiles, const CompilationDatabase &compilations, ASTList &ASTs) {
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
/// \note Templated to accept both unique_ptr and shared_ptr
template <typename smart_ptr>
static void consumeASTs(vector<smart_ptr> &ASTs, unique_ptr<ASTConsumer> consumer, XCallback &cb) {
    for (auto &AST : ASTs) {
        cb.setRewriter(llvm::make_unique<Rewriter>(AST->getSourceManager(), AST->getLangOpts()));
        consumer->HandleTranslationUnit(AST->getASTContext());
        cb.fileProcessed(AST->getSourceManager().getMainFileID(), AST->getMainFileName());
    }
}

// Many types of Matchers, so use a template to support them all
template <typename MatcherType>
void X::transform(const SourceList &sourceFiles, const CompilationDatabase &compilations, MatcherType &matcher,
                  string rhs, bool overwriteChangedFiles) {
    
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
void X::transform(const SourceList &sourceFiles, const CompilationDatabase &compilations, MatcherType &matcher, XCallback &cb) {
    
    ASTList ASTs;
    buildASTs(sourceFiles, compilations, ASTs);
    MatchFinder finder;
    finder.addMatcher(matcher, &cb);
    
    consumeASTs(ASTs, finder.newASTConsumer(), cb);
}

void X::transform(SourceList sourceFiles, const CompilationDatabase &compilations, string LHSTemplateConfigFile) {
    LHSConfiguration lhsConfig(LHSTemplateConfigFile);
    
    // Ensure the template source file also gets parsed
    if (find(sourceFiles.begin(), sourceFiles.end(), lhsConfig.getTemplateSource()) != sourceFiles.end()
        && compilations.getCompileCommands(lhsConfig.getTemplateSource()).empty()) {
        llvm::errs() << "Template source file is not contained in the source list or the compilation database!\n";
        return;
    }
    
    // Parse to ASTs
    // Parse the ASTs and convert the AST list from unique_ptrs to shared_ptrs as one of the ASTs (the template source)
    // may need to be shared across the AST consumer and the LHS template
    // It might be more efficient to not convert the smart pointers in case the AST does not need to be shared,
    // but that incurs more overhead for allowing multiple possible types, forcing us to use even more (C++) templates
    // While doing this, also verify that the template source was parsed correctly and save the pointer for later usage.
    vector<shared_ptr<ASTUnit>> ASTs;
    shared_ptr<ASTUnit> templateSourceAST;
    bool templateSourceParsed = false;
    
    // Use a new scope so we don't confuse the shared and unique pointer AST lists
    {
        ASTList UniqueASTs;
        buildASTs(sourceFiles, compilations, UniqueASTs);
        
        for (auto &uniqueAST : UniqueASTs) {
            shared_ptr<ASTUnit> sharedAST(move(uniqueAST));
            
            if (sharedAST->getMainFileName() == lhsConfig.getTemplateSource()) {
                templateSourceParsed = true;
                templateSourceAST = sharedAST;
                
                // If we don't want to transform the template, don't add it now
                if (!lhsConfig.shouldTransformTemplateSource()) continue;
            }
            
            ASTs.push_back(sharedAST);
        }
    }
    
    if (!templateSourceParsed) {
        llvm::errs() << "Template source file failed to parse\n";
        return;
    }
    
    LHSParserConsumer consumer(lhsConfig);
    consumer.HandleTranslationUnit(templateSourceAST->getASTContext());
    
    /*RHSTemplate rhs(lhsConfig.getRHSTemplate());
    InternalCallback cb(rhs, false);
    consumeASTs(ASTs, llvm::make_unique<LHSParserConsumer>(lhsConfig), cb);*/
}


// Explicit initialization of templates so we can still split header and source files
template void X::transform<StatementMatcher>(const SourceList &sourceFiles, const CompilationDatabase &compilations, StatementMatcher &matcher, string rhs, bool overwriteChangedFiles);
template void X::transform<DeclarationMatcher>(const SourceList &sourceFiles, const CompilationDatabase &compilations, DeclarationMatcher &matcher, string rhs, bool overwriteChangedFiles);
template void X::transform<TypeMatcher>(const SourceList &sourceFiles, const CompilationDatabase &compilations, TypeMatcher &matcher, string rhs, bool overwriteChangedFiles);
template void X::transform<TypeLocMatcher>(const SourceList &sourceFiles, const CompilationDatabase &compilations, TypeLocMatcher &matcher, string rhs, bool overwriteChangedFiles);
template void X::transform<NestedNameSpecifierMatcher>(const SourceList &sourceFiles, const CompilationDatabase &compilations, NestedNameSpecifierMatcher &matcher, string rhs, bool overwriteChangedFiles);
template void X::transform<NestedNameSpecifierLocMatcher>(const SourceList &sourceFiles, const CompilationDatabase &compilations, NestedNameSpecifierLocMatcher &matcher, string rhs, bool overwriteChangedFiles);
template void X::transform<CXXCtorInitializerMatcher>(const SourceList &sourceFiles, const CompilationDatabase &compilations, CXXCtorInitializerMatcher &matcher, string rhs, bool overwriteChangedFiles);

// Same for LHS matchers, RHS callback version of transform
template void X::transform<StatementMatcher>(const SourceList &sourceFiles, const CompilationDatabase &compilations, StatementMatcher &matcher, XCallback &cb);
template void X::transform<DeclarationMatcher>(const SourceList &sourceFiles, const CompilationDatabase &compilations, DeclarationMatcher &matcher, XCallback &cb);
template void X::transform<TypeMatcher>(const SourceList &sourceFiles, const CompilationDatabase &compilations, TypeMatcher &matcher, XCallback &cb);
template void X::transform<TypeLocMatcher>(const SourceList &sourceFiles, const CompilationDatabase &compilations, TypeLocMatcher &matcher, XCallback &cb);
template void X::transform<NestedNameSpecifierMatcher>(const SourceList &sourceFiles, const CompilationDatabase &compilations, NestedNameSpecifierMatcher &matcher, XCallback &cb);
template void X::transform<NestedNameSpecifierLocMatcher>(const SourceList &sourceFiles, const CompilationDatabase &compilations, NestedNameSpecifierLocMatcher &matcher, XCallback &cb);
template void X::transform<CXXCtorInitializerMatcher>(const SourceList &sourceFiles, const CompilationDatabase &compilations, CXXCtorInitializerMatcher &matcher, XCallback &cb);
