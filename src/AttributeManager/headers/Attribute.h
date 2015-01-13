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
#ifndef _ATTRIBUTE_H
#define _ATTRIBUTE_H

#include "Errors.h"
#include "Debug.h"
#include "TwoWayList.h"
#include "TwoWayList.cc"
#include "JsonAST.h"

#include <string>

/** This class defines what an attribute is in the catalog. It contains
 * the following information:
 *
 *  - Attribute name.
 *  - Attribute type.
 *  - Attribute index.
 *  - Primary key information.
 *  - Foreign key information.
 *
 * Keep in mind that an instance of this class can be incomplete. For
 * example, it can contain the attribute name, but not the type nor
 * the index. Or, it can contain just the index. This is necessary in
 * order to allow for flexibility with operations like schema
 * projections and unions, so that those methods just receive a
 * collection of Attribute types and not things like integers
 * (indices) or strings (att. names).
 *
 * Take a look at the description of the "state" variables, as they
 * define how "complete" an instance of this class is.
 **/
class Attribute {

    private:
        // Attribute name.
        std::string name;

        // Attribute type.
        std::string type;

        Json::Value jType;

        // Attribute index inside the schema
        int index;

        // Number of unique values for this attribute
        long int uniques;

        // If so, then which relation and attributes?
        std::string foreignRel;
        std::string foreignAtt;

        // *THIS IS IMPORTANT* -- these variable store essential state
        // information about the current instance. Like having a name, type,
        // index and so on.
        bool haveName;
        bool haveType;
        bool haveIndex;
        bool haveUniques;

        // Is this attribute part of the primary key?
        bool isPrimaryKey;

        // Is this attribute part of a foreign key?
        bool isForeignKey;

        // Forbid default constructors and assignment operations
        Attribute(const Attribute &) = delete;

        Attribute &operator=(const Attribute &) = delete;

        // The schema class is my friend.
        friend class Schema;

    public:
        // Empty constructor -- for collections
        Attribute();

        // Constructor with just a name
        Attribute(std::string _name);

        // Constructor with just an index
        Attribute(int _index);

        // Constructor with name, type, index and number of uniques
        Attribute(std::string _name, std::string _type, Json::Value _jType, int _index, long int _uniques);

        // destructor
        virtual ~Attribute() {}

        // Get/set for attribute name
        std::string GetName() const;
        void SetName(std::string name);
        bool HasName() const;

        // Get/set for attribute type
        std::string GetType() const;
        void SetType(std::string  &in);

        Json::Value GetJType() const;
        void SetJType( Json::Value &in);

        // Get/set for attribute index
        int GetIndex();
        void SetIndex(int idx);

        // Get/set for the number of unique values for this attribute
        long int GetUniques();
        void SetUniques(long int idx);

        // Get/set for primary key
        bool IsPrimaryKey();
        void SetPrimaryKey(bool val);

        // Get/sets for foreign key info
        bool IsForeignKey();
        std::string GetForeignRelation();
        std::string GetForeignAttribute();
        void SetForeignKey(std::string relation, std::string attribute);

        // Copies the contents of this instance.
        void CopyTo(Attribute &out) const;

        // swaps the contents of *this with the parameter instance.
        void swap(Attribute &with);

        // Prints attribute information -- for debugging
        void Print();

        // Transform to/from JSON
        void toJson(Json::Value & dest) const;
        void fromJson(const Json::Value & src);
};

void ToJson( const Attribute & src, Json::Value & dest );
void FromJson( const Json::Value & src, Attribute & dest );

// list of attributes to talk to the Catalog
typedef TwoWayList<Attribute> AttributeContainer;

#endif // _ATTRIBUTE_H
