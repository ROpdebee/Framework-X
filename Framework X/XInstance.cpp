//
//  XInstance.cpp
//  Framework X
//
//  Created by Ruben Opdebeeck on 22/04/2017.
//  Copyright © 2017 Ruben Opdebeeck. All rights reserved.
//

#include "XInstance.hpp"

using namespace X;

XInstance::XInstance() {
    // Diagnostics engine
    _ci.createDiagnostics();
    
    // Target Info
    std::shared_ptr<clang::TargetOptions> pto = std::make_shared<clang::TargetOptions>();
    pto->Triple = llvm::sys::getDefaultTargetTriple();
    clang::TargetInfo *pti = clang::TargetInfo::CreateTargetInfo(_ci.getDiagnostics(), pto);
    _ci.setTarget(pti);
    
    // Source Manager, Preprocessor
    _ci.createFileManager();
    _ci.createSourceManager(_ci.getFileManager());
    _ci.createPreprocessor(clang::TU_Complete);
    
    // Set up the source reader
    _sr = new SourceReader(&_ci.getSourceManager(), _ci.getLangOpts());
    
    // Set up the lexer
    _lex = new Lexer(&_ci);
}
