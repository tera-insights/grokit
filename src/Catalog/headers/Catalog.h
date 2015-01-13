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
#ifndef _CATALOG_H
#define _CATALOG_H

#include <string>
#include <vector>
#include <iostream>
#include <map>
#include <strings.h>
#include <mutex>

#include "Schema.h"
#include "Debug.h"
#include "Errors.h"
#include "Constants.h"
#include "ContainerTypes.h"
#include "EventProcessor.h"
#include "json.h"

/** Macro to ease access to constants used in the system */

// Place the value of the constant "nameConstant" into variable
// If the constant not in the catalog add it with deflt value
#define GET_CATALOG_INT( variable, nameConstant, deflt) \
{ Catalog& cat = Catalog::GetCatalog();                                 \
    variable = cat.GetIntegerConstantWithDefault(nameConstant, deflt);  \
}

#define GET_CATALOG_DOUBLE( variable, nameConstant, deflt) \
{ Catalog& cat = Catalog::GetCatalog();                                 \
    variable = cat.GetDoubleConstantWithDefault(nameConstant, deflt);   \
}

#define GET_CATALOG_STRING( variable, nameConstant, deflt) \
{ Catalog& cat = Catalog::GetCatalog();                                 \
    variable = cat.GetStringConstantWithDefault(nameConstant, deflt);   \
}

/** This is the catalog. This massive class is a container for schema
 * and system configuration information. It provides all the necessary
 * interfaces for maintaining relational schema information and system
 * constants, which are grouped by data type.
 *
 * The actual catalog is stored on disk in XML format. This class is
 * responsible for reading the XML file and filling all the internal
 * data structures with the respective information. Additionally, it
 * provides the interfaces for modifying the catalog and writing it
 * back to disk in XML format so that it can be loaded in another
 * session.
 *
 * An important fact about this class is how it is constructed and
 * maintained across the system. There is supposed to be only one
 * instance of the Catalog class among the system, and this classes
 * enforces so by implementing a singleton pattern. This implies it
 * doesn't have a constructor but rather a GetCatalog() method that
 * the programmer uses to obtain the one and only instance of this
 * class.
 *
 * Since several threads might access and modify the contents of this
 * class at the same time, it is implemented in a thread-safe manner
 * using mutexes.
 **/
class Catalog {

    private:
        typedef std::mutex mutex_t;
        typedef std::unique_lock<mutex_t> unique_lock_t;
        typedef std::lock_guard<mutex_t> scoped_lock_t;
//Catalog(std::string xmlFile);

        // Map of table aliases
        std::map<std::string, std::string> tableAliases;

        // Map of integer constants
        std::map<std::string, int> intConstants;

        // Map of double constants
        std::map<std::string, double> doubleConstants;

        // Map of string constants
        std::map<std::string, std::string> stringConstants;

        // Mutex for constant information
        mutable  mutex_t constMutex;

        // Disk paths
        StringContainer diskPaths;

        // Mutex for disk paths
        mutable mutex_t diskMutex;

        // Collection of schemas
        SchemaContainer schemas;

        // Mutex for schema information
        mutable mutex_t schemaMutex;

        // the one instance
        static Catalog *instance;

        // This is a private method to find a schema by its name
        bool FindSchemaByName(std::string name);

        // Forbid copy constructors and assignment operators
        Catalog(const Catalog &){};
        Catalog &operator=(const Catalog &) { return(*this); }

        // detault constructor is private so only the singleton mechanism works
        Catalog();
    public:
        // Returns the only existing, single instance of the catalog.
        // If the instance does *not* exist, it will read it from the
        // parameter name.
        static Catalog& GetCatalog(void);

        // Sends an update message to the updateLogger
        void SendUpdate(void);

        // Returns an integer constant
        // WARNING: this fails if not value inside
        int GetIntegerConstant(std::string varName);
        // get the value from catalog. If no value, set the value as default
        // and return default
        int GetIntegerConstantWithDefault(std::string varName, int value);

        // Sets an integer constant
        void SetIntegerConstant(std::string varName, int value);

        // Returns a double constant
        double GetDoubleConstant(std::string varName);
        double GetDoubleConstantWithDefault(std::string varName, double value);

        // Sets a double constant
        void SetDoubleConstant(std::string varName, double value);

        // Returns a string constant
        std::string GetStringConstant(std::string varName);
        std::string GetStringConstantWithDefault(std::string varName, std::string value);

        // Sets a string constant
        void SetStringConstant(std::string varName, std::string value);

        // Returns the number of available disks.
        int GetNumDisks();

        // Returns the path of a given disk.
        std::string GetDiskPath(unsigned int whichDisk);

        // Resets the disk path container
        void SetDiskPaths(StringContainer disks);

        // Returns a list of relation names.
        StringContainer GetRelationNames();

        // Returns the schema of a given relation.
        bool GetSchema(std::string relName, Schema &outSchema);
        bool GetSchema(Schema &outSchema);

        // Adds a new schema to the catalog, if already there just replace
        // it.
        void AddSchema(Schema &inSchema);

        // Adds an alias to a table name
        void AddSchemaAlias(std::string from, std::string to);

        // Removes a schema from the catalog
        void RemoveSchema(std::string name);

        // Writes the contents of the catalog back to disk
        void SaveCatalog(void);

        // destructor
        virtual ~Catalog();

        // Transform to/from JSON
        void toJson(Json::Value & dest) const;
        void fromJson(const Json::Value & src);
};

void ToJson( const Catalog & src, Json::Value & dest );
void FromJson( const Json::Value & src, Catalog & dest );

#endif
