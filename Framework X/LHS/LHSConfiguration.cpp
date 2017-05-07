//
//  LHSConfiguration.cpp
//  Framework X
//
//  Created by Ruben Opdebeeck on 03/05/2017.
//  Copyright © 2017 Ruben Opdebeeck. All rights reserved.
//

#include "LHSConfiguration.hpp"

using namespace X;

/// Check constraints on source ranges in the config file
/// We require that all ranges are well-formed (end-point <= start-point),
/// metavariable ranges are inside the template range,
/// and metavariable ranges do not overlap.
/// Throws a descriptive exception when a constraint is found unsatisfied
static void validateRangeConstraints(TemplateRange &templateRange, vector<MetavarLoc> &metavars) {
    
    // Check if the main template range is valid
    if (!templateRange.valid())
        throw MalformedConfigException("Invalid template range!");
    
    // Check metavariables, use chasing pointers
    MetavarLoc prevMetavar;
    for (MetavarLoc meta : metavars) {
        
        if (!meta.range.valid())
            throw MalformedConfigException("Invalid source range for metavariable " + meta.identifier);
        
        if (!meta.range.enclosedIn(templateRange))
            throw MalformedConfigException("Source range for metavariable " + meta.identifier + " falls outside template range");
        
        // Since we know the range of prevMetavar starts before the range of this metavariable,
        // we do not need to check against any other range
        if (prevMetavar.isValid() && prevMetavar.range.overlapsWith(meta.range))
            throw MalformedConfigException("Source ranges for metavariables " + prevMetavar.identifier + " and " + meta.identifier + " overlap");
        
        prevMetavar = meta;
    }
}

/// Parse and validate the JSON configuration
/// Throws an exception on invalid JSON files
static json parseAndValidate(string configPath) {
    
    json schema;
    json config;
    ifstream fSchema(schemaPath);
    ifstream fConfig(configPath);
    json_validator validator(nullptr, nullptr);
    
    // Parse the JSON schema and instantiate the validator
    try {
        fSchema >> schema;
        validator.set_root_schema(schema);
    } catch (const exception &e) {
        cerr << "Unable to instantiate schema validator: " << e.what() << endl;
        throw;
    }
    
    // Parse the config file and validate it
    try {
        fConfig >> config;
        validator.validate(config); // May throw an exception, handle in caller
    } catch (const exception &e) {
        cerr << "Unable to parse configuration" << endl;
        throw(MalformedConfigException(e.what()));
    }
    
    return config;
}

/// \brief Turn a possibly relative path into an absolute path
/// Throws an exception whenever there is an issue converting the path,
/// or when the file does not exist.
static string getAbsolutePath(string path) {
    static auto fs(clang::vfs::getRealFileSystem());
    string absolute;
    error_condition ok;
    
    // Turn the path into an absolute path if necessary
    if (!llvm::sys::path::is_absolute(path)) {
        llvm::SmallVector<char, 128> pathVector;
        for (char &ch : path) pathVector.push_back(ch);
        
        // Check that there were no errors after making the path absolute
        error_code error(fs->makeAbsolute(pathVector));
        if (error != ok) throw MalformedConfigException(error.message());
        
        absolute.reserve(pathVector.size());
        for (char &ch : pathVector) absolute += ch;
    } else absolute = path;
    
    if (!fs->exists(absolute)) throw MalformedConfigException("File " + path + "does not exist");
    
    return absolute;
}

LHSConfiguration::LHSConfiguration(string jsonCfgPath) {
    
    // Don't catch any thrown exceptions, configuration will not make sense if the file is invalid
    json cfg(parseAndValidate(jsonCfgPath));
    
    // Simple data-types
    // This may throw exceptions, forward them to the caller as the config will be invalid
    templateSource = getAbsolutePath(cfg["templateSource"].get<string>());
    rhsTemplate = getAbsolutePath(cfg["rhsTemplate"].get<string>());
    
    if (cfg.find("transformTemplateSource") != cfg.end()) {
        transformTemplateSource = cfg["transformTemplateSource"].get<bool>();
    } else {
        transformTemplateSource = true;
    }
    
    // Complex data-types
    auto jTemplateRange(cfg["templateRange"]);
    templateRange = TemplateRange(jTemplateRange[0], jTemplateRange[1]);
    
    // The JSON interface returns us a std::map from a string to a two-element vector of source locations
    // We need to convert this to a vector of pairs, with each pair having a string and a range
    for (auto &kv : cfg["metaVariables"]) {
        metavariableRanges.push_back(MetavarLoc(kv["identifier"], TemplateRange(kv["range"][0], kv["range"][1])));
    }
    
    // Now sort the vector on its ranges to allow for efficient range constraint checking.
    // We consider range [x1, y1] to be smaller than range [x2, y2] when x1 <= x2
    // This might also be better for cache locality, but that effect will be barely noticable
    // We could have performed this sort while transforming the map in the previous step,
    // but std::map does not support iterating a specific order, and a sorted vector has
    // bad memory efficiency
    std::sort(metavariableRanges.begin(), metavariableRanges.end(),
              [] (MetavarLoc m1, MetavarLoc m2) { return m1.range.begin <= m2.range.begin; });
    
    // Finally, check the range constraints.
    // Our JSON schema cannot handle this, hence we need to do it manually
    // This may again throw an exception, do not handle it because the config is invalid
    validateRangeConstraints(templateRange, metavariableRanges);
}

inline ostream& operator<<(ostream &out, TemplateLocation const &loc) {
    return out << "[" << loc.line << ", " << loc.column << "]";
}

inline ostream& operator<<(ostream &out, TemplateRange const &tr) {
    return out << tr.begin << " -> " << tr.end;
}

void LHSConfiguration::dumpConfiguration() {
    cerr << "Template source file: " << templateSource << endl
    << "RHS template: " << rhsTemplate << endl
    << "Template range: " << templateRange << endl
    << "Metavariables: " << endl;
    for (auto &meta : metavariableRanges) {
        cerr << "\t" << meta.identifier << ": " << meta.range << endl;
    }
}
