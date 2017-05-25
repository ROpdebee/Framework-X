//
//  SourceReader.cpp
//  Framework X
//
//  Created by Ruben Opdebeeck on 22/04/2017.
//  Copyright © 2017 Ruben Opdebeeck. All rights reserved.
//

#include "SourceReader.hpp"

using namespace X;

std::string SourceReader::readSourceRange(const SourceRange sr, const SourceManager &sm) const {
    auto pBegin = sm.getCharacterData(sr.getBegin());
    auto length = sm.getCharacterData(sr.getEnd()) - sm.getCharacterData(sr.getBegin()) + 1;
    return std::string(pBegin, length);
}

std::string SourceReader::readSourceRange(const SourceRange sr) const {
    return readSourceRange(sr, _sm);
}

std::string SourceReader::readSourceRange(const DynTypedNode &node, const SourceManager &sm) const {
    SourceRange sr(node.getSourceRange());
    // Make sure to extend the source range to the end of the last token
    sr.setEnd(clang::Lexer::getLocForEndOfToken(sr.getEnd(), 1, sm, _lops));
    
    // Also include the trailing semicolon
    SourceLocation trailingSemi = X::Lexer::getSemiAfterLocation(sr.getEnd(), sm, _lops);
    if (trailingSemi.isValid()) {
        sr.setEnd(trailingSemi);
    }
    
    return clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(sr), sm, _lops);
}

std::string SourceReader::readSourceRange(const DynTypedNode &node) const {
    return readSourceRange(node, _sm);
}

std::string SourceReader::readSourceRangeIncludingSemi(SourceRange sr, const SourceManager &sm) const {
    // Make sure to extend the source range to the end of the last token
    sr.setEnd(clang::Lexer::getLocForEndOfToken(sr.getEnd(), 1, sm, _lops));
    
    // Also include the trailing semicolon
    SourceLocation trailingSemi = X::Lexer::getSemiAfterLocation(sr.getEnd(), sm, _lops);
    if (trailingSemi.isValid()) {
        sr.setEnd(trailingSemi);
    }
    
    return clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(sr), sm, _lops);

}
