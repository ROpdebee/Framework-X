//
//  LHSTemplate.cpp
//  Framework X
//
//  Created by Ruben Opdebeeck on 07/05/2017.
//  Copyright © 2017 Ruben Opdebeeck. All rights reserved.
//

#include "LHSTemplate.hpp"

void LHSTemplate::addTemplateSubtree(StmtOrDecl subtree) {
    _templateSubtrees.push_back(subtree);
}

void LHSTemplate::addMetavariable(string metavarId, StmtOrDecl subtree) {
    _metavariables.insert({ subtree, metavarId });
}

bool LHSTemplate::isMetavariable(StmtOrDecl subtree) {
    return _metavariables.find(subtree) != _metavariables.end();
}

string LHSTemplate::getMetavariableId(StmtOrDecl subtree) {
    auto it = _metavariables.find(subtree);
    assert(it != _metavariables.end() && "Not a subtree");
    return it->second;
}

void LHSTemplate::dump() {
    llvm::outs() << "Template subtrees:\n~~~~~~~~~~~~~~~~~~\n\n";
    for (auto &subtree : _templateSubtrees) {
        subtree.dump();
        llvm::outs() << "\n\n";
    }
    
    llvm::outs() << "Metavariables:\n~~~~~~~~~~~~~~\n\n";
    for (auto &metaPair : _metavariables) {
        llvm::outs() << metaPair.second << ": " << metaPair.first.getRawPtr() << "\n";
        metaPair.first.dump();
        llvm::outs() << "\n\n";
    }
    
}
