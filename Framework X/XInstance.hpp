//
//  XInstance.hpp
//  Framework X
//
//  Created by Ruben Opdebeeck on 22/04/2017.
//  Copyright © 2017 Ruben Opdebeeck. All rights reserved.
//

#ifndef XInstance_hpp
#define XInstance_hpp

#include <llvm/Support/Host.h>
#include <llvm/ADT/IntrusiveRefCntPtr.h>

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Basic/TargetOptions.h>
#include <clang/Basic/TargetInfo.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/LangOptions.h>

#include "Lexer.hpp"
#include "SourceReader.hpp"

namespace X {

/// \class XInstance
/// \brief An instance of framework X, containing all common parts of transformation.
class XInstance {
    clang::CompilerInstance _ci; ///< The encapsulated compiler instance
    Lexer *_lex; ///< The lexer
    SourceReader *_sr; ///< The source reader
    
public:
    /// Construct an instance of framework X
    XInstance();
    ~XInstance() { delete _sr; delete _lex; }
    
    /// Retrieve the lexer
    Lexer *lexer() { return _lex; }
    
    /// Retrieve the source reader
    SourceReader *sourceReader() { return _sr; }
    
    /// Retrieve the source manager
    clang::SourceManager *sourceManager() { return &_ci.getSourceManager(); }
    
    /// Retrieve the language options
    clang::LangOptions *getLangOpts() { return &_ci.getLangOpts(); }

};
    
} // namespace X

#endif /* XInstance_hpp */
