//
//  RHSTemplate.hpp
//  Framework X
//
//  Created by Ruben Opdebeeck on 20/04/2017.
//  Copyright © 2017 Ruben Opdebeeck. All rights reserved.
//

#ifndef RHSTemplate_hpp
#define RHSTemplate_hpp

#include <string>
#include <iostream>

#include "llvm/ADT/SmallVector.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/AST/ASTTypeTraits.h"
#include "clang/ASTMatchers/ASTMatchers.h"

#include "XInstance.hpp"
#include "Lexer.hpp"
#include "TemplatePart.hpp"
#include "SourceReader.hpp"

// Assume RHS templates generally contain only 10 or less parts
#define TEMPLATE_PARTS_LENGTH 10

using namespace llvm;
using namespace X;

using clang::ast_matchers::MatchFinder;
using ASTNodeBindings = clang::ast_matchers::internal::BoundNodesMap::IDToNodeMap;
using clang::ast_type_traits::DynTypedNode;

namespace X {

/// \class RHSTemplate
/// \brief Represents a right-hand side template.
class RHSTemplate {
    
    /// \brief The various parts of the template, i.e. string literals and metavariables to be instantiated.
    ///
    /// We use an LLVM SmallVector for this purpose because generally, RHS templates do not consist of
    /// many parts. In case of an exceptionally large template, this vector can be grown quite efficiently.
    SmallVector<TemplatePart, TEMPLATE_PARTS_LENGTH> _templateParts;
    
    /// Common instance of framework X
    XInstance &_xi;
    
    /// The lexer
    Lexer* _lexer;
    
    /// The source reader
    SourceReader* _sr;

    /// \brief Split a template into its parts
    ///
    /// This will lex the template to find metaparameters and instantiate the templateParts
    /// \param filePath Path to the RHS template file
    void splitTemplate(std::string filePath);
    
public:
    /// \brief Constructor for a RHS template.
    ///
    /// Reads the contents of the given filename and splits these contents to create the template parts.
    /// \param filePath The path to the RHS template file
    /// \param xi The common instance of framework X
    RHSTemplate(std::string filePath, XInstance &xi);
    
    /// \brief Instantiate the RHS template using the provided node bindings.
    ///
    /// \param bindings The result of AST matching using a MatchFinder
    /// \return An instance of the template
    std::string instantiate(const MatchFinder::MatchResult& bindings);
    
    /// \brief Print out the template to the error stream in its parsed form.
    ///
    /// Intended as a debugging method to verify that the template has been lexed correctly.
    void dumpTemplateParts();
};
    
} // namespace X

#endif /* RHSTemplate_hpp */
