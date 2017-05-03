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

/// \class TemplateRange
/// \brief Representation of template source ranges
class TemplateRange {
public:
    int begin, end;
    TemplateRange(int b, int e) : begin(b), end(e) {}
    TemplateRange() : begin(0), end(0) {}
};
    
/// \class MetavarLoc
/// \brief A metavariable identifier and its associated template range
class MetavarLoc {
public:
    TemplateRange range;
    string identifier;
    
    MetavarLoc(string ident, TemplateRange rng) : identifier(ident), range(rng) {};
    MetavarLoc() : identifier(""), range(-1, 0) {};
    
    bool isValid() { return range.begin != -1; }
};

/// \class LHSConfiguration
///
/// \brief Represents the configuration details for a LHS template.
class LHSConfiguration {
    string templateSource; ///< The path of the source file out of which to generate the template
    TemplateRange templateRange; ///< The range of the template itself
    vector<MetavarLoc> metavariableRanges; ///< The metavariables and their associated ranges in the template
    string rhsTemplate; ///< The path to the RHS template to be used with this LHS template
    
public:
    /// Construct a LHS configuration from the given JSON file.
    /// \param jsonCfgPath The path to the JSON configuration file.
    LHSConfiguration(string jsonCfgPath);
    
    /// Dump the configuration onto the error stream
    /// Intended for debugging purposes
    void dumpConfiguration();
};

}


#endif /* LHSConfiguration_hpp */