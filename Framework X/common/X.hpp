//
//  X.hpp
//  Framework X
//
//  Created by Ruben Opdebeeck on 22/04/2017.
//  Copyright © 2017 Ruben Opdebeeck. All rights reserved.
//

#ifndef X_hpp
#define X_hpp

#include <string>
#include <vector>
#include <fstream>

#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/raw_os_ostream.h>

#include "../RHS/RHSTemplate.hpp"

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;

namespace X {

/// \class XCallback
/// \brief A class implementing a callback for AST matching
/// \see MatchFinder::MatchCallback
///
/// Derive this class and override the `run` method to implement your own behaviour after matches have been found
class XCallback : public MatchFinder::MatchCallback {
protected:
    std::unique_ptr<Rewriter> _pRewriter; ///< Pointer to a rewriter configured to rewrite the currently processed source file
public:
    XCallback() : _pRewriter(nullptr) {}
    
    /// Assign a new rewriter to this callback. Called whenever a new source file is entered.
    /// \param newRewriter Pointer to the new rewriter
    virtual inline void setRewriter(std::unique_ptr<Rewriter> newRewriter) {
        _pRewriter = std::move(newRewriter);
    }
    
    /// Called whenever the current file is fully processed.
    /// \param fid The FileID of the file that has been processed
    /// \param filePath The path to the file that has been processed
    virtual void fileProcessed(FileID fid, std::string filePath) =0;
    
    /// Called whenever a match has been found
    /// \param res The result of the match
    virtual void run(const MatchFinder::MatchResult& res) =0;
};

/// \brief Transform a source file using AST matchers at the LHS and templates at the RHS.
///
/// LHS matching will be performed by conventional AST match finders. A RHS template will be
/// instantiated for each match and the replacement will be applied onto the original source file.
/// \param sourceFiles The source files to be transformed.
/// \param compilations The compilation database.
/// \param matcher The LHS matcher.
/// \param rhs The path to the RHS template.
/// \param overwriteChangedFiles If true, the transformation will overwrite changed files. If false, changes will be written to a new file.
template <typename MatcherType>
void transform(const std::vector<std::string> &sourceFiles, const CompilationDatabase &compilations, MatcherType &matcher,
               std::string rhs, bool overwriteChangedFiles = false);
    
/// \brief Transform a source file using AST matchers at the LHS and a custom callback at the RHS
///
/// LHS matching will be performed by conventional AST match finders. The provided callback will be called on each match.
/// This isn't really different to the normal declarative AST matching approach, but allows you to use the Rewriter instead of Replacements
/// without having to go through the trouble of making it work.
/// \param sourceFiles The source files to be transformed.
/// \param compilations The compilation database.
/// \param matcher The LHS matcher.
/// \param cb The custom callback
template <typename MatcherType>
void transform(const std::vector<std::string> &sourceFiles, const CompilationDatabase &compilations, MatcherType &matcher, XCallback &cb);
    
} // namespace X

#endif /* X_hpp */
