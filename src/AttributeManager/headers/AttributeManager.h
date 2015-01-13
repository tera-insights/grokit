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
#ifndef _ATTRIBUTEMANAGER_H_
#define _ATTRIBUTEMANAGER_H_

#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <sstream>
#include <algorithm>
#include <mutex>

#include "ID.h"
#include "Errors.h"
#include "AttributeInfoInternal.h"
#include "ContainerTypes.h"
#include "AttributeInfo.h"
#include "JsonAST.h"

// How may slots are reserved (indicates first available one)
// The researved slots are used by the Chunks for special purposes
// This number has to be in sync with the macros in Chunk.h
#define FIRST_NONRESERVED_SLOT 2


// forward definitions
class AttributeInfo;


/**
 * This class maintains the information about attributes present in the base
 * relations as well sythesized attributes. It maintains information such as
 * relation to which attribute belong, its slots and type.
 **/

class AttributeManager {

    private:
        typedef std::map<std::string, AttributeInfoInternal*> AttributeNameInfoMap;
        typedef std::map<SlotID, std::string> AttributeSlotToNameMap;
        typedef std::map<std::string, SlotToSlotMap*> RelationToSlotsMap;
        typedef std::map<QueryID, StringContainer> QueryIDToAttributesMap;
        typedef std::vector<bool> Slots;

        // Mutex for the AttributeManager
        typedef std::mutex              Mutex;
        typedef std::lock_guard<Mutex>  ScopedLock;
        typedef std::unique_lock<Mutex> UniqueLock;
        Mutex attributeManagerMutex;

        // keeping only one instance of the class
        static AttributeManager* instance;

        // holds mapping between each attribute name and its information
        AttributeNameInfoMap myAttributes;

        // reverse map from SlotID to name
        AttributeSlotToNameMap reverseMap;

        //holds mapping between a relation and SlotToSlotMap of its columns
        RelationToSlotsMap relationToSlots;

        // holds mapping between query and its synthesized attributes
        QueryIDToAttributesMap queryIdToAttributes;

        // keep track of the available slots
        Slots slotUsage;

        // holds slot of first synthesized attribute
        int firstSynthSlot;

        //////////////////
        // private methos

        // Constructor; loads propogated attributes
        AttributeManager(void);

        // find an empty slot to map the attributes into
        // from starts at 0 not at firstSynthSlot
        int NextEmptySlot(int from = 0);

        // read all the attributes for the base relations from the catalog
        // no reason for this to be public
        // this function has to be called before any synthesized attributes are inserted
        void FillInAttributesFromCatalog(void);

    public:
        // Destructor
        virtual ~AttributeManager(void);

        static AttributeManager& GetAttributeManager();

        /////////////////////////
        // METHODS TO ADD ATTRIBUTES

        // allocate slot for synthesized attribute
        // return the slot allocated
        SlotID AddSynthesizedAttribute(QueryID id, std::string attName, std::string attType, Json::Value jType);

        // allocate slots for synthesized attributes
        // void AddSynthesizedAttributes(QueryID& id, StringContainer& atts);

        //////////////////////////
        // METHODS TO GET INFORMATION ABOUT INDIVIDUAL ATTRIBUTES

        // retrieves the slot for given attribute
        SlotID GetAttributeSlot(std::string attLongName);
        SlotID GetAttributeSlot(std::string tableName, std::string attributeName);

        // return empty string if not found (do not fail)
        std::string GetAttributeType(std::string attLongName);
        std::string GetAttributeType(std::string tableName, std::string attributeName);

        Json::Value GetAttributeTypeJSON( std::string attLongName );
        Json::Value GetAttributeTypeJSON( std::string tableName, std::string attributeName );

        // retrieves the slot for all attributes
        void GetAttributesSlots(std::string tableName, SlotContainer& where);

        // retrieves the slot for given sythesized attribute
        SlotID GetAttributeSlot(QueryID id, std::string attributeName);

        // retrieves the column slot for given attribute
        SlotID GetAttributeColumn(std::string tableName, std::string attributeName);
        SlotID GetAttributeColumn(std::string attLongName);

        // retrieve the name of an attribute by SlotID
        std::string GetAttributeName(SlotID slot);

        // Alias all attributes from tablename to alias, creating slotsw if needed,
        // and returning the slots for the aliased attributes
        void AliasAttributesSlots(std::string tableName, std::string alias, SlotContainer& where);

        /////////////////////////
        // METHODS TO GET INFORMATION ABOUT GROUPS OF ATTRIBUTES

        // extract information about the attributes in the system
        void GetInfoOnAllAttributes(AttributeInfoContainer& where);

        // returns column to slot mapping for given relation
        // the result is placed in where
        void GetColumnToSlotMapping(std::string tableName, SlotToSlotMap& where);

        // deletes synthesized attributes present in system for given query id
        void DeleteAttributes(QueryID queryid);

        // Generates a JSON description of all of the attributes in the system.
        void GenerateJSON( Json::Value & fillMe );

        friend class AttributeInfo;
};

#endif
