//
//  X.hpp
//  Framework X
//
//  Created by Ruben Opdebeeck on 22/04/2017.
//  Copyright © 2017 Ruben Opdebeeck. All rights reserved.
//

#ifndef X_hpp
#define X_hpp

#include <string>
#include <vector>

#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include "RHSTemplate.hpp"
#include "XInstance.hpp"

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;

namespace X {

/// \brief Transform a source file using AST matchers at the LHS and templates at the RHS.
///
/// LHS matching will be performed by conventional AST match finders. A RHS template will be
/// instantiated for each match and the replacement will be applied onto the original source file.
/// \param sourceFiles The source files to be transformed.
/// \param compilations The compilation database.
/// \param matcher The LHS matcher.
/// \param rhs The path to the RHS template.
template <typename MatcherType>
void transform(std::vector<std::string> sourceFiles, const CompilationDatabase &compilations, MatcherType matcher, std::string rhs);
    
} // namespace X

#endif /* X_hpp */
