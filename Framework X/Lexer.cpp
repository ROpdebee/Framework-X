//
//  Lexer.cpp
//  Framework X
//
//  Created by Ruben Opdebeeck on 20/04/2017.
//  Copyright © 2017 Ruben Opdebeeck. All rights reserved.
//

#include "Lexer.hpp"

using namespace X;

void Lexer::beginLexing(std::string filePath) {
    
    // Get a file entry for this source file
    const clang::FileEntry *pFile = _ci->getFileManager().getFile(filePath);
    
    // Enter this file
    _ci->getSourceManager().setMainFileID(_ci->getSourceManager().createFileID(pFile, clang::SourceLocation(), clang::SrcMgr::C_User));
    _ci->getPreprocessor().EnterMainSourceFile();
    _ci->getDiagnosticClient().BeginSourceFile(_ci->getLangOpts(), &_ci->getPreprocessor());
}

void Lexer::endLexing() {
    _ci->getDiagnosticClient().EndSourceFile();
}

bool Lexer::lex(clang::Token &tok) {
    _ci->getPreprocessor().Lex(tok);
    return tok.isNot(clang::tok::eof);
}
