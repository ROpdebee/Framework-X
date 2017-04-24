//
//  TemplatePart.hpp
//  Framework X
//
//  Created by Ruben Opdebeeck on 20/04/2017.
//  Copyright © 2017 Ruben Opdebeeck. All rights reserved.
//

#ifndef TemplatePart_hpp
#define TemplatePart_hpp

#include <string>

namespace X {

/// \class TemplatePart
/// \brief Represents a part of a template
///
/// A template part can either be a string literal or a metavariable.
class TemplatePart {
public:
    
    /// Possible types of a template part
    enum TYPE {
        LITERAL,        ///< A string literal
        METAVARIABLE    ///< A metaparameter
    };
    
    const TYPE type; ///< The type of the template part
    
    /// The content of the template part
    /// In case of a literal, this will be the source content
    /// In case of a metaparameter, this will be the name of the parameter
    const std::string content;
    
    /// Construct a template part
    /// \param type_ The type of the part
    /// \param content_ The content of the part
    TemplatePart(TYPE type_, std::string content_) : type(type_), content(content_) {}
    
    /// Construct an empty template part
    TemplatePart();
};

} // namespace X

#endif /* TemplatePart_hpp */
