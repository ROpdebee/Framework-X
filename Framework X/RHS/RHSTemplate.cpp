//
//  RHSTemplate.cpp
//  Framework X
//
//  Created by Ruben Opdebeeck on 20/04/2017.
//  Copyright © 2017 Ruben Opdebeeck. All rights reserved.
//

#include "RHSTemplate.hpp"

using clang::Token;

/// \brief Check if the given tokens form a metaparameters
///
/// Metaparameters are formatted as ?name.
/// ? are special tokens which are normally not part of an identifier,
/// hence we need to check the previous token as well.
/// We require that there is no whitespace between the two tokens
/// \param curr The current, most recently lexed token.
/// \param prev The previously lexed token.
/// \return True if this token is a metaparameter, false otherwise.
bool isMetaparameter(Token curr, Token prev) {
    return curr.isAnyIdentifier() && prev.is(clang::tok::TokenKind::question)
        && !curr.hasLeadingSpace() && !curr.isAtStartOfLine();
}

RHSTemplate::RHSTemplate(std::string filePath) {
    
    // Setup compiler instance
    _ci.createDiagnostics();
    
    // Target Info
    auto targetOpts = std::make_shared<clang::TargetOptions>();
    targetOpts->Triple = llvm::sys::getDefaultTargetTriple();
    clang::TargetInfo *targetInfo = clang::TargetInfo::CreateTargetInfo(_ci.getDiagnostics(), targetOpts);
    _ci.setTarget(targetInfo);
    
    _ci.createFileManager();
    _ci.createSourceManager(_ci.getFileManager());
    _ci.createPreprocessor(clang::TU_Complete);
    
    _lexer = new Lexer(_ci.getFileManager(), _ci.getSourceManager(), _ci.getDiagnosticClient(),
                       _ci.getPreprocessor(), _ci.getLangOpts());
    _sr = new SourceReader(_ci.getSourceManager(), _ci.getLangOpts());
    parse(filePath);
}

void RHSTemplate::parse(std::string filePath) {
    
    _lexer->beginLexing(filePath);
    
    // Iteratively lex the template into the current token until the template is processed
    // Use both the current and previous token to detect metaparameters
    // Keep track of the source range for literal parts
    Token curr;
    Token prev;
    const SourceManager &sm(_ci.getSourceManager());
    SourceRange literalRange(sm.getLocForStartOfFile(sm.getMainFileID()));
    
    while (_lexer->lex(curr)) {
        
        // When we encounter a metaparameter, read the current source range as a template literal
        // and push both this literal and the metaparameter onto the parts list
        if (isMetaparameter(curr, prev)) {
            
            literalRange.setEnd(prev.getLocation().getLocWithOffset(-1)); // Don't include the question mark in the literal
            _templateParts.push_back(RHSTemplatePart(RHSTemplatePart::LITERAL, _sr->readSourceRange(literalRange)));
            _templateParts.push_back(RHSTemplatePart(RHSTemplatePart::METAVARIABLE, curr.getIdentifierInfo()->getName()));
            
            // Start a new literal range
            literalRange.setBegin(curr.getEndLoc());
        }
        
        // Advance the literal range to the end of the current token
        // In case of a literal token, this is exactly what we need to do
        // In case we just encountered a metaparameter, we started a new range, which is initially empty
        literalRange.setEnd(curr.getEndLoc());
        
        prev = curr;
    }
    
    // Read the final literal and add this part as well,
    // but only if we don't end on a metavariable (in which case the template range will be empty)
    if (literalRange.getBegin() != literalRange.getEnd()) {
        _templateParts.push_back(RHSTemplatePart(RHSTemplatePart::LITERAL, _sr->readSourceRange(literalRange)));
    }
    
    _lexer->endLexing();
}

std::string RHSTemplate::instantiate(const MatchFinder::MatchResult& bindings) {
    // Iterate over the template parts and concatenate the parts while instantiating the metaparameters
    // Do a simple string concatenation. Using .reserve() on the string to reserve memory in the string buffer
    // may provide better performance, but there is no reasonable way to know how much space we must reserve.
    std::string instantiated = "";
    
    // Use underlying map in the bindings to assure we can handle multiple possible node types
    // They do not share a base class, hence we'll need to use a DynTypedNode
    const ASTNodeBindings nodes(bindings.Nodes.getMap());
    ASTNodeBindings::const_iterator node;
    
    for (RHSTemplatePart part : _templateParts) {
        if (part.type == RHSTemplatePart::LITERAL) instantiated += part.content;
        else {
            node = nodes.find(part.content);
            if (node != nodes.end()) instantiated += _sr->readSourceRange(node->second, *bindings.SourceManager);
            else std::cerr << "No binding for " << part.content << std::endl;
        }
    }
    
    return instantiated;
}

void RHSTemplate::dumpTemplateParts() {
    for (RHSTemplatePart part : _templateParts) {
        if (part.type == RHSTemplatePart::LITERAL) std::cerr << part.content;
        else std::cerr << "<" << part.content << ">";
    }
    std::cerr << std::endl;
}
