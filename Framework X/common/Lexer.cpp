//
//  Lexer.cpp
//  Framework X
//
//  Created by Ruben Opdebeeck on 20/04/2017.
//  Copyright © 2017 Ruben Opdebeeck. All rights reserved.
//

#include "Lexer.hpp"

using namespace X;

void Lexer::beginLexing(std::string filePath) const {
    
    // Get a file entry for this source file
    const FileEntry *pFile = _fileMgr.getFile(filePath);
    
    // Enter this file
    _srcMgr.setMainFileID(_srcMgr.createFileID(pFile, SourceLocation(), clang::SrcMgr::C_User));
    _prep.EnterMainSourceFile();
    _diag.BeginSourceFile(_langOpts, &_prep);
}

void Lexer::endLexing() const {
    _diag.EndSourceFile();
}

bool Lexer::lex(Token &tok) const {
    _prep.Lex(tok);
    return tok.isNot(clang::tok::eof);
}

SourceLocation Lexer::getSemiAfterLocation(SourceLocation sl, const SourceManager &sm, const LangOptions &lops) {
    Token tok;
    bool failure = clang::Lexer::getRawToken(sl.getLocWithOffset(1), tok, sm, lops, /*IgnoreWhitespace=*/true);
    
    if (failure || tok.isNot(clang::tok::semi)) return SourceLocation();
    else return tok.getLocation();
}

SourceLocation Lexer::getEndOfLiteral(SourceLocation sl, const SourceManager &sm, const LangOptions &lops) {
    // Take offset=1 to point to the last character in the literal
    return clang::Lexer::getLocForEndOfToken(sl, /*Offset=*/1, sm, lops);
}
