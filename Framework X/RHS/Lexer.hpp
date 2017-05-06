//
//  Lexer.hpp
//  Framework X
//
//  Created by Ruben Opdebeeck on 20/04/2017.
//  Copyright © 2017 Ruben Opdebeeck. All rights reserved.
//

// Adapted from https://github.com/loarabia/Clang-tutorial/wiki/TutorialOrig

#ifndef Lexer_hpp
#define Lexer_hpp

#include <string>

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/Token.h>
#include <clang/Lex/Preprocessor.h>

using clang::FileManager;
using clang::SourceManager;
using clang::DiagnosticConsumer;
using clang::Preprocessor;
using clang::FileEntry;
using clang::SourceLocation;
using clang::LangOptions;
using clang::Token;

namespace X {

/// \class Lexer
/// \brief A common abstraction for Lexer
class Lexer {
    FileManager &_fileMgr;
    SourceManager &_srcMgr;
    DiagnosticConsumer &_diag;
    Preprocessor &_prep;
    LangOptions &_langOpts;
    
public:
    Lexer(FileManager &fMgr, SourceManager &sMgr, DiagnosticConsumer &diag, Preprocessor &prep, LangOptions &lops)
        : _fileMgr(fMgr), _srcMgr(sMgr), _diag(diag), _prep(prep), _langOpts(lops) {};
    
    /// Set up the Lexer to start lexing the given file.
    /// \param filePath Path to the file to be lexed.
    void beginLexing(std::string filePath) const;
    
    /// End lexing the current file.
    void endLexing() const;
    
    /// Lex a token in the current file.
    /// \param tok This token will be destructively modified to contain the newly lexed token.
    /// \return True if the file still has tokens to read, false if the lexed token is the EOF token.
    bool lex(clang::Token &tok) const;

};

} // namespace X
#endif /* Lexer_hpp */
