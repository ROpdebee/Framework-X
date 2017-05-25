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

using namespace X;

static llvm::cl::OptionCategory ToolCategory("C++ Template Transformation Tool");

int main(int argc, const char **argv) {
    clang::tooling::CommonOptionsParser op(argc, argv, ToolCategory);
    
    try {
        X::transform(op.getSourcePathList(), op.getCompilations(), "config.json");
    } catch (const MalformedConfigException& e) {
        llvm::errs() << e.what() << "\n";
    }
    
    return 0;
}
