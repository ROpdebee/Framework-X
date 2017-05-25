//
//  ASTTraversalState.cpp
//  Framework X
//
//  Created by Ruben Opdebeeck on 19/05/2017.
//  Copyright © 2017 Ruben Opdebeeck. All rights reserved.
//

#include "ASTTraversalState.hpp"

using namespace X;

long ASTNode::nextID = 0;
ASTNodeKind ASTNode::StmtKind = ASTNodeKind::getFromNodeKind<Stmt>();
ASTNodeKind ASTNode::DeclKind = ASTNodeKind::getFromNodeKind<Decl>();
ASTNodeKind ASTNode::DeclStmtKind = ASTNodeKind::getFromNodeKind<DeclStmt>();
ASTNodeKind ASTNode::FunctionDeclKind = ASTNodeKind::getFromNodeKind<FunctionDecl>();
ASTNodeKind ASTNode::VarDeclKind = ASTNodeKind::getFromNodeKind<VarDecl>();
ASTNodeKind ASTNode::FieldDeclKind = ASTNodeKind::getFromNodeKind<FieldDecl>();

vector<ASTNode> &ASTNode::getChildren() {
    // If the child list has been instantiated, just return it
    if (childrenAdded) return children;
    
    ASTNodeKind nodeKind(node.getNodeKind());
    
    // We still need to instantiate the child list
    // Depending on the type of the underlying node, children are accessed in different ways
    if (StmtKind.isBaseOf(nodeKind)) {
        // For Stmts, in the general case the children are accessed using its children() range.
        // However, for DeclStmts, we need to access them by its decls() range.
        if (DeclStmtKind.isBaseOf(nodeKind)) {
            for (const Decl *D : node.get<DeclStmt>()->decls()) {
                children.push_back(ASTNode(DynTypedNode::create(*D)));
            }
        } else {
            for (const Stmt *child : node.get<Stmt>()->children()) {
                if (child) children.push_back(ASTNode(DynTypedNode::create(*child)));
                else children.push_back(ASTNode());
            }
        }
    } else {
        // For Decls, in the general case, children are accessed using the decls() range of DeclContext, if that Decl derives it.
        // For FunctionDecls and its derivations, we need to create two children, one for its parameter list (virtual child) and one for its body.
        // For VarDecls and its derivations and FieldDecls, the only possible child is a Stmt, namely its initializer.
        // For any other Decl that does not derive a DeclContext, there are no children.
        
        // FunctionDecls
        if (FunctionDeclKind.isBaseOf(nodeKind)) {
            // Parameter list
            vector<ASTNode> params;
            const FunctionDecl *FD(node.get<FunctionDecl>());
            for (ParmVarDecl *P : FD->parameters()) {
                params.push_back(ASTNode(DynTypedNode::create(*P)));
            }
            children.push_back(ASTNode(params)); // Virtual ASTNode
            
            // Body if there is one (declaration with definition)
            if (FD->isThisDeclarationADefinition()) {
                children.push_back(ASTNode(DynTypedNode::create(*FD->getBody())));
            }
        }
        
        // VarDecl/FieldDecl/ParmVarDecl
        // The only child is the initializer
        else if (VarDeclKind.isBaseOf(nodeKind) && node.get<VarDecl>()->hasInit()) { // Also works for ParmVarDecl
            children.push_back(ASTNode(DynTypedNode::create(*node.get<VarDecl>()->getInit())));
        } else if (FieldDeclKind.isBaseOf(nodeKind) && node.get<FieldDecl>()->hasInClassInitializer()) {
            children.push_back(ASTNode(DynTypedNode::create(*node.get<FieldDecl>()->getInClassInitializer())));
        }
        
        // DeclContext
        else if (llvm::isa<DeclContext>(node.get<Decl>())) {
            for (Decl *child : cast<DeclContext>(node.get<Decl>())->decls()) {
                children.push_back(ASTNode(DynTypedNode::create(*child)));
            }
        }
        
        // Anything else does not have children
    }
    
    childrenAdded = true;
    return children;
}

vector<ASTNode> ASTNode::fromParentAndChild(DynTypedNode &parent, DynTypedNode &child) {
    // Create an ASTNode from the parent and get its children
    ASTNode parentASTNode(parent);
    auto children(parentASTNode.getChildren());
    
    // Find the child we want
    auto childIt(children.end());
    for (auto it = children.begin(); it != children.end(); it++) {
        if (it->getNode() == child) {
            childIt = it;
            break;
        }
    }
    
    // Add the relevant children, i.e. the ones starting from the child we want, to a vector
    vector<ASTNode> relevantChildren;
    vector<ASTNode> results;
    for (; childIt < children.end(); childIt++) {
        relevantChildren.push_back(*childIt);
        results.push_back(ASTNode(relevantChildren)); // Use the vector copy to our advantage
    }
    
    // Create a virtual ASTNode and return it
    return results;
}

bool ASTTraversalState::isLastChild() {
    return currNodeIdx + 1 == parents.top().getChildren().size();
}

bool ASTTraversalState::astProcessed() {
    return parents.empty();
}

bool ASTTraversalState::hasChildren() {
    return parents.top().getChildren()[currNodeIdx].getChildren().size() != 0;
}

bool ASTTraversalState::childrenAccessed() {
    return parents.top().getChildren()[currNodeIdx].areChildrenAccessed();
}

DynTypedNode &ASTTraversalState::getCurrent() {
    return parents.top().getChildren()[currNodeIdx].getNode();
}

DynTypedNode &ASTTraversalState::backtrackToParent() {
    ASTNode parent(parents.top());
    parents.pop();
    
    // Search the parent in its parent, set the new currNodeIdx to its index
    // Only if there are parents left
    if (!parents.empty()) {
        auto siblings(parents.top().getChildren());
        for (unsigned i = 0; i < siblings.size(); i++) {
            if (siblings[i] == parent) {
                currNodeIdx = i;
                break;
            }
        }
    }
    
    return parent.getNode();
}

DynTypedNode &ASTTraversalState::nextSibling() {
    if (isLastChild()) throw runtime_error("No more siblings");
    
    return parents.top().getChildren()[++currNodeIdx].getNode();
}

DynTypedNode &ASTTraversalState::descendToChild() {
    if (!hasChildren()) throw runtime_error("No children");
    
    auto &curr(parents.top().getChildren()[currNodeIdx]);
    curr.setChildrenAccessed(true);
    parents.push(curr);
    currNodeIdx = 0;
    return curr.getChildren()[0].getNode();
}

vector<DynTypedNode> PotentialMatch::getMatchRoot() {
    vector<DynTypedNode> rootList;
    vector<ASTNode> &subtrees(root.getChildren());
    for (ASTNode &node : subtrees) {
        rootList.push_back(node.getNode());
    }
    return rootList;
}

void PotentialMatch::instantiateCurrentAsMetavariable(Metavariable &meta) {
    metavarInstantiations.insert(pair<Metavariable, ASTNode>(meta, parents.top().getChildren()[currNodeIdx]));
}

void PotentialMatch::extendForMetavariable(Metavariable &meta, vector<PotentialMatch> &potentials) {
    auto &siblings(parents.top().getChildren());
    vector<ASTNode> instanceNodes;
    // The vector passed to the constructor of ASTNode is passed by value, i.e. copied, so we can
    // destructively modify it at each step and give a copy to each new potential match that is created
    for (unsigned i = currNodeIdx; i < siblings.size(); i++) {
        instanceNodes.push_back(siblings[i]);
        PotentialMatch newMatch(*this);
        ASTNode instance(instanceNodes);
        newMatch.metavarInstantiations.insert(pair<Metavariable, ASTNode>(meta, instance));
        newMatch.currNodeIdx = i; // Set the current node to the last node in the instantiation 
        potentials.push_back(newMatch);
    }
}
