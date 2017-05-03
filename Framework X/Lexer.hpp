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

using clang::CompilerInstance;

namespace X {

/// \class Lexer
/// \brief A common abstraction for Lexer
class Lexer {
    CompilerInstance* _ci; ///< The encapsulated CompilerInstance that provides the Lexer
    
public:
    Lexer(CompilerInstance *ci) : _ci(ci) {};
    
    /// Set up the Lexer to start lexing the given file.
    /// \param filePath Path to the file to be lexed.
    void beginLexing(std::string filePath);
    
    /// End lexing the current file.
    void endLexing();
    
    /// Lex a token in the current file.
    /// \param tok This token will be destructively modified to contain the newly lexed token.
    /// \return True if the file still has tokens to read, false if the lexed token is the EOF token.
    bool lex(clang::Token &tok);

};

} // namespace X
#endif /* Lexer_hpp */
