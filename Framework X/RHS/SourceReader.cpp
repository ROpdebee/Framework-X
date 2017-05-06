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
    auto buffer = sm.getCharacterData(sr.getBegin());
    auto length = sm.getCharacterData(sr.getEnd()) - sm.getCharacterData(sr.getBegin());
    return std::string(buffer, length);
}

std::string SourceReader::readSourceRange(const SourceRange sr) const {
    return readSourceRange(sr, _sm);
}

std::string SourceReader::readSourceRange(const DynTypedNode &node, const SourceManager &sm) const {
    // Make sure to extend the source range to the end of the last token
    SourceRange sr(node.getSourceRange().getBegin(), clang::Lexer::getLocForEndOfToken(node.getSourceRange().getEnd(), 0, sm, _lops));
    return readSourceRange(sr, sm);
}

std::string SourceReader::readSourceRange(const DynTypedNode &node) const {
    return readSourceRange(node, _sm);
}
