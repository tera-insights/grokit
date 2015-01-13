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

#ifndef _SCHEMA_H
#define _SCHEMA_H

#include "Attribute.h"
#include "Debug.h"
#include "Errors.h"
#include "TwoWayList.h"
#include "TwoWayList.cc"
#include "json.h"

#include <string>

/** This class stores schema infromation about a particular relation.
 * Essentially, the attributes that compose it (a collection of Attribute
 * objects), the name of the relation, the path to the metadata file
 * and the number of tuples in the relation.
 *
 * Additionally, functionality for projection and schema union is provided.
 **/
class Schema {

    private:
        // Collection of attributes
        AttributeContainer attributes;

        // Relation name
        std::string relName;

        // Path to the metadata information
        std::string metadataPath;

        // Number of tuples
        long long int numTuples;

        // Attribute the relation is clustered upon
        Attribute clusterAttribute;

        // Find an attribute based on its name
        bool FindAttByName(std::string name);

        // Find an attribute based on its index
        bool FindAttByIndex(int idx);

        // Forbid default constructors and assignment operators
        Schema(const Schema &) { }

        Schema &operator=(const Schema &) { return(*this); }

        // The catalog class is my friend.
        friend class Catalog;

    public:

        // Empty constructor -- for collections
        Schema() { }

        // Constructor with name only -- for Catalog search
        Schema(std::string _relName):
            relName(_relName) { }

        // destructor
        virtual ~Schema() { }

        // Complete constructor based on attributes, name, metadata path and
        // number of tuples -- the attribute container is left unusable.
        Schema(AttributeContainer &atts, std::string relName, std::string metadataPath,
                long int numTuples, Attribute& clusterAtt);

        // Get/Set for the relation name
        std::string GetRelationName();
        void SetRelationName(std::string relName);

        // Get/Set for the metadata path
        std::string GetMetadataPath();
        void SetMetadataPath(std::string path);

        // Get/Set for the number of tuples
        int GetNumTuples();
        void SetNumTuples(long long int tuples);

        // Returns a *copy* of the entire collection of attributes
        void GetAttributes(AttributeContainer &attsIn);

        // Returns information for a given attribute
        bool GetAttribute(std::string attName, Attribute &who);
        bool GetAttribute(Attribute &who);

        // Sets information for an existing attribute, if existing --
        // otherwise, replace it
        void AddAttribute(Attribute &who);

        // Returns whether or not this Schema is clustered on an attribute
        bool IsClustered() const;

        // Sets the clustering attribute to `name`
        bool SetClusterAttribute(const std::string& name);

        // If the schema is clustered, sets `where` to the clustering attribute
        // and returns true, otherwise returns false without modifying `where`
        bool GetClusterAttribute(Attribute &where) const;

        // Returns a collection of attributes that define the primary key
        void GetPrimaryKey(AttributeContainer &who);

        // Returns a collection of attributes that are part of a foreign key
        void GetForeignKeys(AttributeContainer &who);

        // Projects a set of attributes from this schema
        void Project(AttributeContainer &toKeep);

        // Removes a set of attributes from this schema
        void Remove(AttributeContainer &toRemove);

        // Swap two schemas
        void swap(Schema &with);

        // Copy contents in another schema
        void CopyTo(Schema &here);

        // Transform to/from JSON
        void toJson(Json::Value & dest) const;
        void fromJson(const Json::Value & src);
};

void ToJson( const Schema & src, Json::Value & dest );
void FromJson( const Json::Value & src, Schema & dest );

// container for Schemas
typedef TwoWayList<Schema> SchemaContainer;


// Inlined methods

inline std::string Schema::GetRelationName() {
    return(relName);
}

inline void Schema::SetRelationName(std::string _relName) {
    relName = _relName;
}

inline std::string Schema::GetMetadataPath() {
    return(metadataPath);
}

inline void Schema::SetMetadataPath(std::string _metadataPath) {
    metadataPath = _metadataPath;
}

inline int Schema::GetNumTuples() {
    return(numTuples);
}

inline void Schema::SetNumTuples(long long int _numTuples) {
    numTuples = _numTuples;
}

#endif // _SCHEMA_H
