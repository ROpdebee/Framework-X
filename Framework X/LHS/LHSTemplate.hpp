//
//  LHSTemplate.hpp
//  Framework X
//
//  Created by Ruben Opdebeeck on 07/05/2017.
//  Copyright © 2017 Ruben Opdebeeck. All rights reserved.
//

#ifndef LHSTemplate_hpp
#define LHSTemplate_hpp

#include "LHSTemplateParser.hpp"

using namespace std;
using namespace X;

namespace X {
    
class StmtOrDecl;
using SubtreeList = vector<StmtOrDecl>;
    
/// \class LHSTemplate
/// \brief Represents a LHS template
class LHSTemplate {
    SubtreeList _templateSubtrees;
    
    /// Keep a mapping from subtrees (Stmts and Decls) of the main template to the metavariables
    /// We cannot create new AST nodes easily, nor can we alter them, so there is no way for us
    /// to embed metavariable information directly in the template AST. Hence, when comparing
    /// ASTs to the template, we must check if the AST node in the template is in this map to
    /// determine whether or not it is a metavariable.
    /// Note that a metavariable may be mapped to multiple times if it represents multiple subtrees,
    /// this is fine because the template doesn't need to know what is underneath the metavariable.
    map<StmtOrDecl, string> _metavariables;
    
public:
    LHSTemplate() {}
    
    /// Append an AST subtree to the end of the template subtree list
    void addTemplateSubtree(StmtOrDecl subtree);
    
    /// Add a metavariable to the template
    /// \param metavarId The identifier of the metavariable
    /// \param subtree The subtree this metavariable corresponds to in the main template
    void addMetavariable(string metavarId, StmtOrDecl subtree);
    
    /// Determine whether or not a subtree is represented by a metavariable in the template
    bool isMetavariable(StmtOrDecl subtree);
    
    /// Retrieve the identifier of the metavariable representing this subtree
    string getMetavariableId(StmtOrDecl subtree);
    
    /// Dump the template. Used for debugging purposes
    void dump();
};

} // namespace X

#endif /* LHSTemplate_hpp */
