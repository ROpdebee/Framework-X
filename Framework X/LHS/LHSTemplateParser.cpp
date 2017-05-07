//
//  LHSTemplateParser.cpp
//  Framework X
//
//  Created by Ruben Opdebeeck on 07/05/2017.
//  Copyright © 2017 Ruben Opdebeeck. All rights reserved.
//

#include "LHSTemplateParser.hpp"

using namespace X;

// A template may span multiple AST subtrees,
// provided it spans them entirely. Otherwise,
// the template range is invalid.
//
// Visually:
//
// Allowed:
// ~~~~~~~
//      The template contains multiple subtrees
//  Template:   [................]
//  Subtrees:   [.....][.....][..]
//
//      The template contains one subtree
//  Template:   [................]
//  Subtree:    [................]
//
//      The template is nested deeper inside the subtree
//  Template:       [.....]
//  Subtree:    [................]
//
// Not allowed:
// ~~~~~~~~~~~
//      The template partially overlaps with subtrees
//  Template:       [.......]
//  Subtrees:   [.......][.......]
//
// AST view:
//      Node1         Node2
//     /    \         /   \
//    /   |-------------|  \
//   /    |   \     /   |   \
//  Sub1  | Sub2   Sub3 |   Sub4
//        |-------------|
//          Template
//
// This could cause a template to be constructed as the following:
//                                      |--------|
//                              expr1 + | expr2; |
//      expr1 + expr2;        |---------|        |
//      int decl1;      =>    | int decl1;       |
//      int decl2;            | int decl2;       |
//                            |------------------|
//
// This applies to metavariables as well

template <class DeclOrStmt>
bool LHSParserVisitor::matchSubtreeToRange(DeclOrStmt *subtree, const TemplateRange &range) {
    
    // Ignore statements in included files, carry on with the next AST subtree
    if (!_sm.isWrittenInMainFile(subtree->getLocStart())) return true;
    
    // The start and end location of the node, as written in the file before preprocessing
    TemplateLocation locStart(TemplateLocation::fromSourceLocation(subtree->getLocStart(), _sm));
    TemplateLocation locEnd(TemplateLocation::fromSourceLocation(subtree->getLocEnd(), _sm));
    TemplateRange sourceRange(locStart, locEnd);
    
    // If the template range of this node comes after the range of this subtree,
    // then don't search any further in any subtree of this node and continue to
    // search in the next subtrees
    //  Template:           [........]
    //  Subtree:    [...]
    if (sourceRange.end < _templateSourceRange.begin) {
        return true;
    }
    
    // If the source range of this node falls completely behind the template range we're looking for,
    // then abort the search as we overshot the template and couldn't find it (subtrees are processed preorder)
    //  Template:   [....]
    //  Subtree:            [....]
    if (sourceRange.begin > _templateSourceRange.end) {
        throw MalformedConfigException("Template overshot: LHS parsing could not match the template range to a valid series of AST subtrees");
    }
    
    // When the start of our source range is the same as the start of the range we're looking for,
    // start the template construction. Don't push the current statement on the queue yet,
    // that will be done further down
    //  Template:   [.............]
    //  Subtree:    [....]
    if (sourceRange.begin == _templateSourceRange.begin) {
        templateConstructionBegan = true;
    }
    
    // We're currently investigating a subtree to add to the template AST
    //  Template:   [.............]
    //  Subtree:       ?[....]?
    if (templateConstructionBegan) {
        
        // If our ending location is greater than the template's,
        // the template spans this subtree only partially and is invalid
        // FIXME: Semicolons and literals may cause the subtree end location to be counterintuitive
        //  Template:   [.............]
        //  Subtree:              [......]
        if (sourceRange.end > _templateSourceRange.end) {
            throw MalformedConfigException("Template only partially spans a subtree");
        }
        
        // We're now definitely part of the template, so we add ourselves to the node queue
        templateSubtrees.push(DynTypedNode::create<DeclOrStmt>(*subtree));
        
        // If our end is also the end of the template, we'll end the template here
        // Also end the traversal, so we'll continue on to match metavariables
        //  Template: [...........]
        //  Subtree:        [.....]
        if (sourceRange.end == _templateSourceRange.end) {
            templateParsed = true;
            return false;
        } else {
            return true;
        }
    }
    
    // If we reach this part, we're sure the template (partially) overlaps with our subtree
    //
    // Case 1:
    // ~~~~~~~
    //  Template:       [......]
    //  Subtree:    [.............]
    //
    // Case 2:
    // ~~~~~~~
    //  Template:   [.........]         ILLEGAL: Template construction should have already begun
    //  Subtree:        [........]               Probably could not find a match in an earlier subtree
    //
    // Case 3:
    // ~~~~~~~
    //  Template:        [........]     ILLEGAL: Partially spanned subtree
    //  Subtree:    [......]
    //
    // Case 4:
    // ~~~~~~~
    //  Template:   [.............]     Already handled earlier
    //  Subtree:    [.....]
    
    // Case 2:
    if (_templateSourceRange.begin < sourceRange.begin) {
        throw MalformedConfigException("Could not find a subtree for the start of the template range");
    }
    
    // Case 3:
    if (_templateSourceRange.end > sourceRange.end) {
        throw MalformedConfigException("Template will partially span a subtree");
    }
    
    // Case 1: Simply descend into the subtree and carry on matching
    return continueTraversal(subtree);
}

template<>
bool LHSParserVisitor::continueTraversal<Decl>(Decl *subtree) {
    return RecursiveASTVisitor<LHSParserVisitor>::TraverseDecl(subtree);
}

template<>
bool LHSParserVisitor::continueTraversal<Stmt>(Stmt *subtree) {
    return RecursiveASTVisitor<LHSParserVisitor>::TraverseStmt(subtree);
}

bool LHSParserVisitor::TraverseStmt(Stmt *S) {
    if (!S) return true; // Ignore empty nodes
    return matchSubtreeToRange(S, _templateSourceRange);
}

bool LHSParserVisitor::TraverseDecl(Decl *D) {
    
    if (!D) return true; // Empty node, continue search
    
    // If we just started a translation unit, don't try to match anything as a declaration unit doesn't have
    // a valid source location. Just start the traversal
    if (D->getKind() == Decl::TranslationUnit) return RecursiveASTVisitor<LHSParserVisitor>::TraverseDecl(D);
    
    return matchSubtreeToRange(D, _templateSourceRange);
}
