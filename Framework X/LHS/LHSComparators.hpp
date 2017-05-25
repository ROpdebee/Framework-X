//
//  LHSComparators.hpp
//  Framework X
//
//  Created by Ruben Opdebeeck on 23/05/2017.
//  Copyright © 2017 Ruben Opdebeeck. All rights reserved.
//

#ifndef LHSComparators_hpp
#define LHSComparators_hpp

#include <clang/AST/ASTTypeTraits.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Decl.h>

#include <llvm/ADT/APFloat.h>

using namespace clang;
using namespace clang::ast_type_traits;

namespace X {

/// Compare a template node to a potential match node, return true if they match, false otherwise.
/// If the third argument is true, the matching will ignore differences in name.
extern bool compare(DynTypedNode templNode, DynTypedNode potMatchNode, bool nameOnly = false);
    
} // namespace X

#endif /* LHSComparators_hpp */
