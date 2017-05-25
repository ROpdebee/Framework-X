//
//  LHSConfiguration.hpp
//  Framework X
//
//  Created by Ruben Opdebeeck on 03/05/2017.
//  Copyright © 2017 Ruben Opdebeeck. All rights reserved.
//

#ifndef LHSConfiguration_hpp
#define LHSConfiguration_hpp

#include <string>
#include <utility>
#include <map>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include <llvm/Support/Path.h>
#include <llvm/ADT/SmallVector.h>
#include <clang/Basic/VirtualFileSystem.h>
#include <clang/Basic/SourceManager.h>

#include <json.hpp>
#include <json-schema.hpp>

#define schemaPath "configSchema.json"

namespace X {

using namespace std;
    
using nlohmann::json;
using nlohmann::json_uri;
using nlohmann::json_schema_draft4::json_validator;

/// \class MalformedConfigException
/// \brief Custom exception to specify configuration problems
class MalformedConfigException : public runtime_error {
public:
    MalformedConfigException(const string message) : runtime_error(message) {};
    // inherit the what() method, let runtime_error handle this
};
    
/// \class TemplateLocation
/// \brief Representation of template source locations
class TemplateLocation {
public:
    int line, column;
    TemplateLocation() : line(0), column(0) {}
    TemplateLocation(int l, int c) : line(l), column(c) {}
    
    inline bool operator==(const TemplateLocation &l) const {
        return line == l.line && column == l.column;
    }
    
    inline bool operator<=(const TemplateLocation &l) const {
        return line < l.line || (line == l.line && column <= l.column);
    }
    
    inline bool operator!=(const TemplateLocation &l) const {
        return !(*this == l);
    }
    
    inline bool operator<(const TemplateLocation &l) const {
        return *this <= l && *this != l;
    }
    
    inline bool operator>=(const TemplateLocation &l) const {
        return !(*this < l);
    }
    
    inline bool operator>(const TemplateLocation &l) const {
        return !(*this <= l);
    }
    
    inline static TemplateLocation dummy() { return TemplateLocation(-1, 0); }
    inline bool isDummy() const { return line == -1; }
    
    inline static TemplateLocation fromSourceLocation(clang::SourceLocation sl, const clang::SourceManager &sm) {
        return TemplateLocation(sm.getSpellingLineNumber(sl), sm.getSpellingColumnNumber(sl));
    }
};

/// \class TemplateRange
/// \brief Representation of template source ranges
class TemplateRange {
public:
    TemplateLocation begin, end;
    TemplateRange(TemplateLocation b, TemplateLocation e) : begin(b), end(e) {}
    TemplateRange() : begin(), end() {}
    
    inline static TemplateRange dummy() { return { TemplateLocation::dummy(), TemplateLocation::dummy() }; }
    inline bool isDummy() const { return begin.isDummy(); }
    
    /// Check if a source range is valid.
    /// A source range is valid if the end location is greater than ("behind") the starting location
    inline bool valid() const {
        return begin <= end;
    }
    
    /// Check if this range and the argument range overlap
    /// It is assumed that ranges are passed in ascending order,
    /// i.e. the starting point of this range is lesser than that of the argument range
    /// Following this assumption, we only need to check that the start of the argument range
    /// does not precede the end of this range
    inline bool overlapsWith(const TemplateRange &other) const {
        return end >= other.begin;
    }
    
    /// Check if this range is enclosed in the argument range
    inline bool enclosedIn(const TemplateRange &outerRange) const {
        return outerRange.begin <= begin && outerRange.end >= end;
    }
    
    inline bool operator==(const TemplateRange &tr) const {
        return begin == tr.begin && end == tr.end;
    }
    
    inline bool operator!=(const TemplateRange &tr) const {
        return !(*this == tr);
    }
    
    /// range1 <= range2 if range1.begin <= range2.begin
    inline bool operator<=(const TemplateRange &tr) const {
        return begin <= tr.begin;
    }
    
    /// range1 < range2 if range1.begin < range2.begin
    inline bool operator<(const TemplateRange &tr) const {
        return begin < tr.begin;
    }
    
    /// range1 >= range2 if range1.begin >= range2.begin
    inline bool operator>=(const TemplateRange &tr) const {
        return begin >= tr.begin;
    }
    
    /// range1 > range2 if range1.begin > range2.begin
    inline bool operator>(const TemplateRange &tr) const {
        return begin > tr.begin;
    }
};

inline ostream &operator<<(ostream &out, const TemplateLocation &tl) {
    return out << "[" << tl.column << ", " << tl.line << "]";
}
inline ostream &operator<<(ostream &out, const TemplateRange &tr) {
    return out << tr.begin << " -> " << tr.end;
}
    
// Custom JSON conversion to TemplateLocations and TemplateRanges
inline void from_json(const json &j, TemplateLocation &loc) {
    loc.line = j[0];
    loc.column = j[1];
}

inline void from_json(const json &j, TemplateRange &range) {
    range.begin = j[0];
    range.end = j[1];
}
    
/// \class Metavariable
/// \brief Represents a LHS metavariable, including its identifier and properties
class Metavariable {
public:
    string identifier;
    bool nameOnly = false; ///< Indicates that for NamedDecl nodes, only the name is parameterized, not the type.
    
    Metavariable(string id) : identifier(id) {}
    
    inline bool operator<(const Metavariable &other) const { return identifier < other.identifier; }
};

    
/// \class MetavarLoc
/// \brief A metavariable and its associated template range
class MetavarLoc : public X::Metavariable {
public:
    TemplateRange range;
    
    MetavarLoc(string ident, TemplateRange rng) : Metavariable(ident), range(rng) {};
    MetavarLoc() : Metavariable(""), range(TemplateRange::dummy()) {};
    
    inline bool isValid() const { return !range.isDummy(); }
    /// Custom comparator to order MetavarLoc's.
    /// This dictates in what order metavariables should be parsed from the source AST.
    /// Metavariables with ranges starting earlier, or with larger ranges if the two ranges
    /// start at the same time, precede metavariables with ranges starting later, or smaller ones.
    /// When the ranges are equivalent, nameOnly metavariables take priority over non-nameOnly metavariables.
    /// If the nameOnly flag is equal on both, they are ordered based on identifier.
    inline bool operator<(const MetavarLoc &other) const {
        if (range == other.range) {
            if (nameOnly == other.nameOnly) return identifier < other.identifier;
            else return nameOnly;
        } else if (range.begin == other.range.begin) {
            return range.end > other.range.end;
        } else {
            return range.begin < other.range.begin;
        }
    }
};

/// \class LHSConfiguration
///
/// \brief Represents the configuration details for a LHS template.
class LHSConfiguration {
    string templateSource; ///< The path of the source file out of which to generate the template
    TemplateRange templateRange; ///< The range of the template itself
    vector<MetavarLoc> metavariableRanges; ///< The metavariables and their associated ranges in the template
    string rhsTemplate; ///< The path to the RHS template to be used with this LHS template
    bool transformTemplateSource; ///< Flag indicating if the template source file must be transformed as well
    bool overwriteSourceFiles; ///< Flag indicating if the transformation should overwrite the original source files
    
public:
    /// Construct a LHS configuration from the given JSON file.
    /// \param jsonCfgPath The path to the JSON configuration file.
    LHSConfiguration(string jsonCfgPath);
    
    /// Dump the configuration onto the error stream
    /// Intended for debugging purposes
    void dumpConfiguration();
    
    const string& getTemplateSource() { return templateSource; }
    const TemplateRange& getTemplateRange() { return templateRange; }
    const vector<MetavarLoc>& getMetavariableRanges() { return metavariableRanges; }
    const string& getRHSTemplate() { return rhsTemplate; }
    bool shouldTransformTemplateSource() { return transformTemplateSource; }
    bool shouldOverwriteSourceFiles() { return overwriteSourceFiles; }
};

}


#endif /* LHSConfiguration_hpp */
