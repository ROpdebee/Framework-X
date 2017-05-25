//
//  ASTTraversalState.hpp
//  Framework X
//
//  Created by Ruben Opdebeeck on 19/05/2017.
//  Copyright © 2017 Ruben Opdebeeck. All rights reserved.
//

#ifndef ASTTraversalState_hpp
#define ASTTraversalState_hpp

#include <vector>

#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/ASTTypeTraits.h>
#include <clang/Frontend/ASTUnit.h>

#include "LHSConfiguration.hpp"

using namespace clang::ast_type_traits;
using namespace clang;
using namespace std;

namespace X {
    
/// \class ASTNode
/// \brief Generic representation of an AST node with its children, in order to facilitate AST traversal.
/// Different AST nodes have different ways to access its children, this class tries to generalize that.
/// An AST node can be virtual, used to separate different child lists when no real AST node exists that
/// achieves that goal, e.g. for FunctionDecls, the parameter list and the function body are two different
/// children, but they are represented in the FunctionDecl using different accessors. A virtual ASTNode
/// allows us to "create" two separate AST nodes, one for the parameter list and one for the function body,
/// and add these as two children of the FunctionDecl.
///
/// Child lists are only instantiated once the children get accessed. This way, no expensive instantiation
/// is performed when this node does not match.
///
/// This class allows us to build and use a hierarchical representation of an AST without having to worry
/// about the different representations of AST nodes and its children.
class ASTNode {
    /// A static counter that gets incremented on each new instance created. Used to assign identifiers
    /// to an instance, so that we can check for equivalence without using pointers and without using
    /// underlying data.
    static long nextID;
    static ASTNodeKind StmtKind;
    static ASTNodeKind DeclKind;
    static ASTNodeKind DeclStmtKind;
    static ASTNodeKind FunctionDeclKind;
    static ASTNodeKind VarDeclKind;
    static ASTNodeKind FieldDeclKind;
    
    DynTypedNode node; ///< The real AST node this object represents, or nullptr when it's a virtual AST node.
    vector<ASTNode> children; ///< A list of children for this AST node, accessed in a general way.
    bool virtualNode; ///< Flag indicating that this node is virtual
    bool childrenAdded = false; ///< Flag indicating that the child list has been instantiated.
    bool childrenAccessed = false; ///< Flag indicating that the traversal has entered a child of this ASTNode.
    long ID;
    
public:
    /// Construct a real ASTNode, representing the given real node.
    /// \param realNode The real AST node encapsulated by this ASTNode
    ASTNode(DynTypedNode realNode) : node(realNode), virtualNode(false), ID(nextID++) {}
    
    /// Construct a virtual ASTNode as a node with a list of children.
    /// \param childList The children of this virtual ASTNode.
    ASTNode(vector<ASTNode> childList) : children(childList), virtualNode(true), childrenAdded(true), ID(nextID++) {}
    
    /// Construct a virtual ASTNode without children, i.e. an empty node.
    ASTNode() : virtualNode(true), childrenAdded(true), ID(nextID++) {}
    
    bool isVirtual() const { return virtualNode; }
    
    bool areChildrenAccessed() const { return childrenAccessed; }
    void setChildrenAccessed(bool flag) { childrenAccessed = flag; }
    
    /// Retrieve the children represented by this ASTNode. If the child list has not been instantiated, this
    /// method will retrieve the children of the underlying node before returning the list.
    vector<ASTNode> &getChildren();
    
    DynTypedNode &getNode() { return node; }
    long getID() { return ID; }
    
    bool operator==(ASTNode other) { return ID == other.ID; }
    
    /// Create an ASTNode from a parent and a child.
    /// This will create a virtual AST node whose children are all children
    /// in the given parent node, starting from the child node.
    /// Children before the given child node are ignored.
    /// The resulting ASTNode's children will be in the same order as originally.
    /// \param parent The parent node whose children will become the children of the new ASTNode.
    /// \param child The first child of parent that will be included in the new ASTNode's children.
    /// \return A list of new ASTNodes, each containing one more child than the previous.
    static vector<ASTNode> fromParentAndChild(DynTypedNode &parent, DynTypedNode &child);
};

/// \class ASTTraversalState
/// \brief Class containing information regarding the state of a traversal in an AST,
///        along with some convenience methods.
/// The initial current node is the first node in the AST.
class ASTTraversalState {
protected:
    /// The (virtual) root of the AST being traversed
    ASTNode root;
    
    /// The index of the current node in the child list of the current parent.
    unsigned currNodeIdx;
    
    /// A stack of parent ASTNodes representing the path down the AST
    stack<ASTNode> parents;
    
public:
    /// Create an ASTTraversalState
    /// \param astRoot The root of the AST to traverse
    ASTTraversalState(ASTNode astRoot) : root(astRoot), currNodeIdx(0) {
        astRoot.setChildrenAccessed(true);
        parents.push(astRoot);
    }
    
    /// Check if the current ASTNode is the last child of its parent.
    bool isLastChild();
    
    /// Check if the AST traversal has traversed through the whole AST.
    bool astProcessed();
    
    /// Retrieve the current node.
    DynTypedNode &getCurrent();
    
    /// Traverse to the next sibling of the current node and return it.
    /// Adjust the current node to be the new node.
    /// Will throw an exception when all siblings have been visited.
    DynTypedNode &nextSibling();
    
    /// Walk back upwards to the parent of the current node.
    /// Adjusts the current node to be the parent.
    DynTypedNode &backtrackToParent();
    
    /// Descend down the AST to the first child of the current node.
    /// Adjusts the current node to be the child.
    /// Will throw an exception if it does not have any children.
    DynTypedNode &descendToChild();
    
    /// Check if the current node has children.
    bool hasChildren();
    
    /// Check if we've descended into the current node's children yet.
    bool childrenAccessed();
    
    long getID() { return parents.top().getChildren()[currNodeIdx].getID(); }
};

/// \class PotentialMatch
/// \brief A class representing a potential match in the LHS template matching algorithm.
/// It derives ASTTraversalState in order to facilitate moving through the potential match.
/// It keeps a pointer to the AST unit that owns the potential match, in order to allow
/// potential matches to be grouped by AST unit later on.
class PotentialMatch : public ASTTraversalState {
    map<Metavariable, ASTNode> metavarInstantiations; ///< A map containing the instantiations for metavariables for a potential match
    shared_ptr<ASTUnit> owningAST; ///< A pointer to the AST that owns this potential match.
    
public:
    PotentialMatch(ASTNode root, shared_ptr<ASTUnit> owner) : ASTTraversalState(root), owningAST(owner) {}
    
    /// Retrieve a list of AST subtrees that make up the match
    vector<DynTypedNode> getMatchRoot();
    
    /// Retrieve the metavariable mappings
    map<Metavariable, ASTNode> &getMetavariables() { return metavarInstantiations; }
    
    /// Take the current node in the AST traversal and instantiate it as the given metavariable.
    void instantiateCurrentAsMetavariable(Metavariable &meta);
    
    /// Take the current node in the AST traversal and create new potential matches with the
    /// given metavariable instantiated. As metavariable instantiations may span multiple AST nodes,
    /// we create new potential matches for each possible sequence of subtrees the metavariable
    /// could instantiate. New potential matches are added to the passed vector.
    void extendForMetavariable(Metavariable &meta, vector<PotentialMatch> &potentials);
    
    /// Retrieve the owning AST of this potential match.
    shared_ptr<ASTUnit> getOwner() { return owningAST; }
};
    
} // namespace X

#endif /* ASTTraversalState_hpp */
