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
    
    /// \brief Return a source location for the semicolon immediately following a source location
    /// \param sl The source location to search a semicolon after
    /// \param sm The source manager managing this source location
    /// \param lops Language options for the source file
    /// \return The source location of the semicolon, or an invalid source location when there is no immediate semicolon
    static SourceLocation getSemiAfterLocation(SourceLocation sl, const SourceManager &sm, const LangOptions &lops);
    
    /// \brief Return the very last source location of a literal value.
    /// \param sl The beginning of the literal
    /// \param sm The source manager managing this source location
    /// \param lops Language options for the source file
    /// \return The source location of the end of the literal value
    /// This method is necessary as literal values often get collapsed into a compact representation
    /// after lexing, e.g. true -> 1, 0b10 -> 2, ...
    static SourceLocation getEndOfLiteral(SourceLocation sl, const SourceManager &sm, const LangOptions &lops);

};

} // namespace X
#endif /* Lexer_hpp */
