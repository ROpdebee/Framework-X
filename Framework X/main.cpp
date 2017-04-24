//
//  main.cpp
//  Framework X
//
//  Created by Ruben Opdebeeck on 11/04/2017.
//  Copyright © 2017 Ruben Opdebeeck. All rights reserved.
//

/*
// Return true if this token is a metavariable for an identifier.
// Identifier placeholders are formatted as ?identifier.
// ? are special tokens which are normally not part of an identifier,
// hence we need to check the previous token as well.
// We require there is no space between the identifier and the question mark.
bool isIdentifierPlaceholder(Token curr, Token prev) {
    return curr.isAnyIdentifier() && prev.is(clang::tok::TokenKind::question) && !curr.hasLeadingSpace();
}

// Return true if this token is a metavariable for an expression.
// Expression placeholders are formatted as $expression.
bool isExpressionPlaceholder(Token curr) {
    return curr.isAnyIdentifier() && curr.getIdentifierInfo()->getName().startswith("$");
}
 */

#include <iostream>

#include "clang/Tooling/CommonOptionsParser.h"

#include "X.hpp"

using namespace clang::ast_matchers;
using namespace X;

static StatementMatcher mtchr(ifStmt(hasCondition(anyOf(binaryOperator(hasOperatorName("=="),
                                                                       hasRHS(ignoringImpCasts(cxxBoolLiteral(equals(true)))),
                                                                       hasLHS(stmt().bind("expr"))),
                                                        binaryOperator(hasOperatorName("!="),
                                                                       hasRHS(ignoringImpCasts(cxxBoolLiteral(equals(false)))),
                                                                       hasLHS(stmt().bind("expr"))))),
                                     hasThen(stmt().bind("body")),
                                     hasElse(stmt().bind("alt"))).bind("root"));

static llvm::cl::OptionCategory ToolCategory("Test");

int main(int argc, const char **argv) {
    clang::tooling::CommonOptionsParser op(argc, argv, ToolCategory);
    
    X::transform(op.getSourcePathList(), op.getCompilations(), mtchr, "templ.tmpl");
    
    return 0;
}
