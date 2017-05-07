//
//  main.cpp
//  Framework X
//
//  Created by Ruben Opdebeeck on 11/04/2017.
//  Copyright © 2017 Ruben Opdebeeck. All rights reserved.
//

#include <iostream>

#include <clang/Tooling/CommonOptionsParser.h>

#include "common/X.hpp"
#include "LHS/LHSConfiguration.hpp"

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
    
    //X::transform(op.getSourcePathList(), op.getCompilations(), mtchr, "templ.tmpl");
    
    X::transform(op.getSourcePathList(), op.getCompilations(), "config.json");
    
    //LHSConfiguration conf("config.json");
    //conf.dumpConfiguration();
    
    return 0;
}
