//
//  Copyright 2012 Alin Dobra and Christopher Jermaine
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// for Emacs -*- c++ -*-

#ifndef _ATTRIBUTETYPE_H
#define _ATTRIBUTETYPE_H

#include "Errors.h"
#include "Debug.h"
#include "DataTypes.h" // for the type description

#include <string>


/** This class defines the type of an attribute in the schema. The
 * actual "type" is an enum encapsulated inside and this class just
 * provides interfacing with several methods that help to serve type
 * compatibility (for example, between numeric types).
 *
 * In order to make the type system as easy to extend as possible,
 * the types supported and the info on how to transform them to/from strings
 * are specified in DataTypes.h
 *
 * There are only 2 ways to create an attributeType object
 * 1. From a string specifying the type (as it appears in the catalog)
 * 2. Using a copy constructor to allow propagation of attributes
 *
 * Direct specification of the type is not allowed in order to
 * maintain the extensibility of the type system.
 *
 * An important method here is GetTypeAsString(), which will return a
 * character string representation of the type, suitable for code
 * generation. Additionally, GetCatalogString() returns a character
 * string that can be used by the catalog.
 **/
class AttributeType {

#include "AttributeTypePrivate.h"

    public:
        // Default constructor -- for collections.
        AttributeType():
            type(TUnknown), size(-1) { }

        // destructor
        virtual ~AttributeType() {}

        // creator function from a string
        // only way to create the type besides copy constructor
        static AttributeType CreateFromName(const char*);

        // oposite function (two versions, C++ and SQL)
        void GetTypeAsStringCPP(char* where);
        void GetTypeAsStringSQL(char* where);

        // method to determine if the attributes are compatible with various operations
        bool IsNumeric(void){ return cat==CNumeric; }
        bool IsDate(void){ return cat==CDate; }
        bool IsString(void){ return cat==CString; }

        // Returns the size associate to the attribute
        int GetSize();

        // SWAPPING PARAGIGM
        void swap(AttributeType& other);
        void copy(AttributeType &to);
};

inline int AttributeType::GetSize() {
    WARNINGIF(size==-1, "Trying to get the size of a non-string attribute!");
    return(size);
}

// Override global swap
void swap( AttributeType & a, AttributeType & b );

#endif // _ATTRIBUTETYPE_H
