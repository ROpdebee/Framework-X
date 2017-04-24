//
//  SourceReader.hpp
//  Framework X
//
//  Created by Ruben Opdebeeck on 22/04/2017.
//  Copyright © 2017 Ruben Opdebeeck. All rights reserved.
//

#ifndef SourceReader_hpp
#define SourceReader_hpp

#include "clang/AST/AST.h"
#include "clang/Lex/Lexer.h"
#include "clang/Basic/SourceManager.h"
#include "clang/AST/ASTTypeTraits.h"

using clang::SourceManager;
using clang::SourceRange;
using clang::LangOptions;
using clang::ast_type_traits::DynTypedNode;

namespace X {
  
/// \class SourceReader
/// \brief Common abstractions for reading source files
class SourceReader {
    /// The global source manager
    SourceManager *_sm;
    
    /// Language options
    LangOptions &_lops;
public:
    
    /// Construct a source reader
    /// \param sm The global source manager
    /// \param lops Language options
    SourceReader(SourceManager *sm, LangOptions &lops) : _sm(sm), _lops(lops) {}
    
    /// \brief Read a source range from a file, given a source range and a source manager.
    ///
    /// Note: Source ranges already include the file to be read.
    /// \param sr The source range to read
    /// \param sm The source manager to use
    /// \return The source code in this range
    std::string readSourceRange(const SourceRange sr, const SourceManager *sm);
    
    /// \brief Read a source range from a file, given a source range.
    ///
    /// Note: Source ranges already include the file to be read.
    /// \param sr The source range to read
    /// \return The source code in this range
    std::string readSourceRange(const SourceRange sr);
    
    /// \brief Read a source range from a file, given an AST node and a source manager.
    /// \param node The AST node to read the source code for
    /// \param sm The source manager to use
    /// \return The source code for this AST node
    std::string readSourceRange(const DynTypedNode &node, const SourceManager *sm);
    
    /// \brief Read a source range from a file, given an AST node.
    /// \param node The AST node to read the source code for
    /// \return The source code for this AST node
    std::string readSourceRange(const DynTypedNode &node);

};

} // namespace X

#endif /* SourceReader_hpp */
