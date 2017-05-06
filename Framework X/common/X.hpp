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
#include <fstream>

#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/raw_os_ostream.h>

#include "../RHS/RHSTemplate.hpp"

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
/// \param overwriteChangedFiles If true, the transformation will overwrite changed files. If false, changes will be written to a new file.
template <typename MatcherType>
void transform(std::vector<std::string> sourceFiles, const CompilationDatabase &compilations, MatcherType matcher,
               std::string rhs, bool overwriteChangedFiles = false);
    
} // namespace X

#endif /* X_hpp */
