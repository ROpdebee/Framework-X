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
