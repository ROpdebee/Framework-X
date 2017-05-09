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

bool LHSParserVisitor::parseSubtreeToTemplate(StmtOrDecl subtree) {
    
    // Ignore statements in included files, carry on with the next AST subtree
    if (!_sm.isWrittenInMainFile(subtree.getLocStart())) return true;
    
    // The start and end location of the node, as written in the file before preprocessing
    TemplateLocation locStart(TemplateLocation::fromSourceLocation(subtree.getLocStart(), _sm));
    TemplateLocation locEnd(TemplateLocation::fromSourceLocation(subtree.getLocEnd(), _sm));
    
    // Expand the range to include the full trailing literal
    // Do this even if this subtree is not a literal, as it may contain a literal
    SourceLocation literalSLoc(Lexer::getEndOfLiteral(subtree.getLocEnd(), _sm, _lops));
    TemplateLocation locEndWithLiteral(TemplateLocation::fromSourceLocation(literalSLoc, _sm));
    
    // Expand the range to include the trailing semicolon, if there is one.
    // This location will be invalid if there is no trailing semi, so take care of that
    SourceLocation semiSLoc(Lexer::getSemiAfterLocation(literalSLoc, _sm, _lops));
    TemplateLocation locEndWithSemi(semiSLoc.isValid() ? TemplateLocation::fromSourceLocation(semiSLoc, _sm) : locEndWithLiteral);
    
    // If the template range of this node comes after the range of this subtree,
    // then don't search any further in any subtree of this node and continue to
    // search in the next subtrees
    //  Template:           [........]
    //  Subtree:    [...]
    if (locEnd < _templateSourceRange.begin) {
        return true;
    }
    
    // If the source range of this node falls completely behind the template range we're looking for,
    // then abort the search as we overshot the template and couldn't find it (subtrees are processed preorder)
    //  Template:   [....]
    //  Subtree:            [....]
    if (locStart > _templateSourceRange.end) {
        throw MalformedConfigException("Template overshot: LHS parsing could not match the template range to a valid series of AST subtrees");
    }
    
    // When the start of our source range is the same as the start of the range we're looking for,
    // start the template construction. Don't push the current statement on the queue yet,
    // that will be done further down
    //  Template:   [.............]
    //  Subtree:    [....]
    if (locStart == _templateSourceRange.begin) {
        templateConstructionBegan = true;
    }
    
    // We're currently investigating a subtree to add to the template AST
    //  Template:   [.............]
    //  Subtree:       ?[....]?
    if (templateConstructionBegan) {
        
        // If our ending location is greater than the template's,
        // the template spans this subtree only partially and is invalid
        //  Template:   [.............]
        //  Subtree:              [......]
        if (locEnd > _templateSourceRange.end) {
            throw MalformedConfigException("Template only partially spans a subtree");
        }
        
        // We're now definitely part of the template, so we add ourselves to the node queue
        templateSubtrees.push(subtree);
        
        // If our end is also the end of the template, we'll end the template here
        // Also end the traversal, so we'll continue on to parse metavariables
        // Make sure to also include semicolons and expanded literals in this check,
        // as they are normally not included in the source range
        //  Template: [...........]
        //  Subtree:        [.....]
        if (locEndWithSemi == _templateSourceRange.end || locEndWithLiteral == _templateSourceRange.end
            || locEnd == _templateSourceRange.end) {
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
    //
    // Case 5:
    // ~~~~~~~
    //  Template:   [........]          Template spans us partially, but may span a subtree completely
    //  Subtree:    [.............]
    
    // Case 2:
    if (_templateSourceRange.begin < locStart) {
        throw MalformedConfigException("Could not find a subtree for the start of the template range");
    }
    
    // Case 3:
    if (_templateSourceRange.end > locEndWithSemi) {
        throw MalformedConfigException("Template will partially span a subtree");
    }
    
    // Case 1, case 5: Simply descend into the subtree and carry on parsing
    return continueTraversal(subtree);
}

bool LHSParserVisitor::parseMetavariables(StmtOrDecl subtree) {
    
    // The start and end location of the subtree, as written in the file before preprocessing
    TemplateLocation locStart(TemplateLocation::fromSourceLocation(subtree.getLocStart(), _sm));
    TemplateLocation locEnd(TemplateLocation::fromSourceLocation(subtree.getLocEnd(), _sm));
    TemplateRange sourceRange(locStart, locEnd);
    
    // Expand the range to include the full trailing literal
    // Do this even if this subtree is not a literal, as it may contain a literal
    SourceLocation literalSLoc(Lexer::getEndOfLiteral(subtree.getLocEnd(), _sm, _lops));
    TemplateLocation locEndWithLiteral(TemplateLocation::fromSourceLocation(literalSLoc, _sm));
    
    // Expand the range to include the trailing semicolon, if there is one.
    // This location will be invalid if there is no trailing semi, so take care of that
    SourceLocation semiSLoc(Lexer::getSemiAfterLocation(literalSLoc, _sm, _lops));
    TemplateLocation locEndWithSemi(semiSLoc.isValid() ? TemplateLocation::fromSourceLocation(semiSLoc, _sm) : locEndWithLiteral);
    
    llvm::outs() << "[" << locStart.line << ", " << locStart.column << "] -> [" << locEndWithSemi.line << ", " << locEndWithSemi.column << "]\n";

    
    // Check all remaining metavariables' ranges to check if this subtree is of importance
    // Keep in mind that metavariable ranges can never overlap, simplifying our task
    bool searchSubtrees(false);
    for (auto &metavar : remainingMetavariables) {
        if (!metavar.range.overlapsWith(sourceRange)) continue; // Current metavariable is not a part of this subtree
        
        // When we start a metavariable, do the necessary bookkeeping
        // Make sure we don't start on metavariables we can't finish, see the template matching above
        if (metavar.range.begin == locStart && locEnd <= metavar.range.end) {
            parsingMetavariable = &metavar;
            parsedMetavariables.insert({ metavar.identifier, SubtreeList() });
            // Do not insert the subtree just yet, it will be done further down
            break; // No need to check the rest of the metavariables, they cannot overlap
        }
        
        // When at least one of our subtrees contains a metavariable, mark it as such
        // If it is not fully enclosed in our subtree, i.e. "a part sticks out",
        // it might partially span a subtree. Don't check for this, the metavar will simply never
        // get parsed or an exception will be thrown elsewhere
        if (metavar.range.enclosedIn(sourceRange)) {
            searchSubtrees = true;
            break;
        }
    }
    
    // When we're in the process of parsing a metavariable, inspect this subtree
    // to see if it can be added to its subtree sequence without creating partially spanned subtrees
    // Very similar to template matching
    if (parsingMetavariable) {
        
        TemplateLocation metaEnd(parsingMetavariable->range.end);
        
        //  Metavar:   [.............]
        //  Subtree:              [......]
        if (locEnd > metaEnd) {
            throw MalformedConfigException("Metavariable only partially spans a subtree");
        }
        
        // We're definitely part of the metavariable's subtree sequence, so we add ourselves to it
        parsedMetavariables[parsingMetavariable->identifier].push_back(subtree);
        
        // If we're the end of the metavariable's subtree sequence, mark this metavariable as done
        if (locEndWithSemi == metaEnd || locEndWithLiteral == metaEnd || locEnd == metaEnd) {
            remainingMetavariables.erase(*parsingMetavariable);
            parsingMetavariable = nullptr;
        }
        
        // Don't descend further down the subtree, we disallow partial subtrees
        // Continue parsing
        return true;
    }
    
    // We're currently not parsing a metavariable, but there may be metavariables in one of our children
    if (searchSubtrees) {
        return continueTraversal(subtree);
    }
    
    // If there is no metavariable in our children, don't search them
    return true;
}

bool LHSParserVisitor::continueTraversal(StmtOrDecl subtree) {
    if (subtree.getKind() == StmtOrDecl::Kind::DECL) return RecursiveASTVisitor<LHSParserVisitor>::TraverseDecl(subtree.getAsDecl());
    else return RecursiveASTVisitor<LHSParserVisitor>::TraverseStmt(subtree.getAsStmt());
}

bool LHSParserVisitor::TraverseStmt(Stmt *S) {
    if (!S) return true; // Ignore empty nodes
    
    // Parse the template or the metavariables
    if (!templateParsed) return parseSubtreeToTemplate(S);
    else return parseMetavariables(S);
}

bool LHSParserVisitor::TraverseDecl(Decl *D) {
    
    if (!D) return true; // Empty node, continue search
    
    // If we just started a translation unit, don't try to parse anything as a declaration unit doesn't have
    // a valid source location. Just start the traversal
    if (D->getKind() == Decl::TranslationUnit) return RecursiveASTVisitor<LHSParserVisitor>::TraverseDecl(D);
    
    // Parse the template or the metavariables
    if (!templateParsed) return parseSubtreeToTemplate(D);
    else return parseMetavariables(D);
}
