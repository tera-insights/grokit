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
#include "AttributeManager.h"
#include "AttributeInfo.h"
#include "QueryManager.h"
#include "Catalog.h"
#include "Errors.h"

#include <iostream>
#include <fstream>
#include <string>

using namespace std;

/** Todo: fix the deletion to erase info from reverseMap */

// instance default value
AttributeManager* AttributeManager::instance = NULL;


AttributeInfoInternal :: AttributeInfoInternal(string _relationName, string _name,
        string _type, Json::Value _jType, int _column, int _slot){
    shortName = _name;
    name = REL_ATT_KEY(_relationName, _name);
    type=_type;
    jType = _jType;

    relationName = _relationName;

    SlotID slotToAdd(_slot);
    slot= slotToAdd;

    SlotID columnToAdd(_column);
    column = columnToAdd;
}


AttributeInfoInternal::AttributeInfoInternal(QueryID& id, string _name,
        string _type, Json::Value _jType, int _slot){
    shortName = _name;
    QueryManager& qm=QueryManager::GetQueryManager();
    string qName;
    if (!qm.GetQueryName(id,qName)){
        FATAL("Query is not registered!");
    }

    // FIXME: Make less magic
    if( qName.length() > 8 ) {
        qName = qName.substr(0, 8);
    }

    // get attribute full name by adding query id to it
    name = SYN_ATT_KEY(qName,_name);
    type=_type;
    jType = _jType;

    relationName = "";
    SlotID slotToAdd(_slot);
    slot = slotToAdd;
}


AttributeManager::AttributeManager() {

    firstSynthSlot = FIRST_NONRESERVED_SLOT;

    //fill the manager with attributes from catalog
    FillInAttributesFromCatalog();
}

AttributeManager::~AttributeManager() {

    //free all the used memory
    myAttributes.clear();
    reverseMap.clear();

    relationToSlots.clear();

    queryIdToAttributes.clear();
}

AttributeManager& AttributeManager::GetAttributeManager() {
    //is instance already created?
    if(instance == NULL)
    {
        //creating it once
        instance = new AttributeManager();
    }
    return *instance;
}

int AttributeManager::NextEmptySlot(int from) {
    // stupid linear scan to find the next empty slot
    // we do not do this too often so this should be fine
    int slot;

    int slotsNum = slotUsage.size();
    for (slot=from; slot<slotsNum; slot++) {
        if (slotUsage[slot] == false)
            break;
    }

    // if slot ends up with value size() than we insert a new element
    // this results in having infinite slots.. Need to change logic, if fixed number
    // of slots are needed
    if(slot == slotsNum) {
        slotUsage.push_back(false);
    }

    // need to compensate for permanently used slots
    return slot+firstSynthSlot;
}

void AttributeManager::FillInAttributesFromCatalog() {
    // the catalog should be initialized by now
    // get access to the catalog
    Catalog& catalog=Catalog::GetCatalog();

    // var to keep track of the next slot to use
    int slot=FIRST_NONRESERVED_SLOT;

    // get the relation names
    StringContainer relNames = catalog.GetRelationNames();

    // iterate over relations
    for (unsigned int i=0; i<relNames.size(); i++) {
        string thisRel = relNames[i];

        // ask for the schema of this relation
        Schema schema;
        catalog.GetSchema(thisRel,schema);

        //creating map for holding attribute's column and slot mapping
        SlotToSlotMap* slotToSlotMap = new SlotToSlotMap();

        //container for holding attributes from schema
        AttributeContainer attributes;
        schema.GetAttributes(attributes);

        attributes.MoveToStart();

        int columnNo = 0;
        // iterate over the attributes from schema.
        // here its assumed that attributes are coming in right order
        // as defined in schema
        while(!attributes.AtEnd()) {
            //get each attribute
            Attribute& attribute = attributes.Current();
            attributes.Advance();

            //retrieve information of each attribute of schema
            string attName = attribute.GetName();
            string attributeType = attribute.GetType();
            Json::Value attJType = attribute.GetJType();

            //now create AttributeInfoInternal from Attribute's information
            AttributeInfoInternal* att = new AttributeInfoInternal(thisRel, attName,
                    attributeType, attJType, columnNo++, slot);
            slot++;

            SlotID slotToAdd(att->Slot());
            SlotID slotCopy = slotToAdd;
            SlotID columnToAdd(att->Column());

            // add into the map key as attribute's column and value as its slot
            slotToSlotMap->Insert(columnToAdd, slotToAdd);

            // create a key of form: relationname_attname
            string attKey = att->Name();

            // add attribute information to the container
            myAttributes[attKey] = att;
            reverseMap[slotCopy] = attKey;
        }

        //insert mapping between the relation name and ColumnToSlot mapping
        relationToSlots.insert(pair<string,SlotToSlotMap*>(thisRel,slotToSlotMap));
    }

    //set the first slot available for synthesized attributes
    firstSynthSlot = slot;
}

SlotID AttributeManager::AddSynthesizedAttribute(QueryID id, string attName,
        string attType, Json::Value jType) {

    SlotID rez; // put the slotID of the attribute added here

    // get the mutex
    ScopedLock guard(attributeManagerMutex);

    //create new attribute for the new synthesized attribute
    AttributeInfoInternal* att = new AttributeInfoInternal(id, attName, attType, jType);

    // get key similar to synthesized attribute name
    string attKey = att->Name();

    // flag for checking if attribute is added to the system
    bool isAttributeAdded = false;

    // find if earlier attributes of same query id were added
    QueryIDToAttributesMap::iterator qtoaMapItr;
    qtoaMapItr = queryIdToAttributes.find(id);

    // if yes, then add this new attribute to that list
    if(qtoaMapItr != queryIdToAttributes.end())
    {
        StringContainer& attributes = (*qtoaMapItr).second;

        // check if attribute with same name is already present for the query id
        if(find(attributes.begin(), attributes.end(), attKey) == attributes.end())
        {
            attributes.push_back(attKey);
            isAttributeAdded = true;
        }
        else
        {
            WARNING("Attribute with the name %s already present for the query id.", attName.c_str());
        }
    }
    else
    {
        // if attributes for this queryid are added for first time,
        // then create a new list of attributes, add this attribute
        //StringContainer* attributes = new StringContainer();
        StringContainer attributes;
        attributes.push_back(attKey);
        // add query id and list as a pair in the map
        QueryID* keyID = new QueryID();
        keyID->copy(id);

        queryIdToAttributes.insert(pair<QueryID,StringContainer>(*keyID,attributes) );
        isAttributeAdded = true;
    }

    // now if attribute is added, then add it to map myAttributes,
    // which maintains all attributes in the system
    if(isAttributeAdded)
    {
        // get next empty slot
        int nextSlot=NextEmptySlot(0);
        SlotID slotToAdd(nextSlot);
        SlotID slotCopy = slotToAdd;
        rez=slotToAdd;

        att->SetSlot(slotToAdd);

        // add attribute information to the container
        myAttributes[attKey] = att;
        reverseMap[slotCopy] = attKey;

        slotUsage.at(nextSlot-firstSynthSlot) = true;
    }

    return rez;
}


SlotID AttributeManager::GetAttributeSlot(string attributeLongName){
    //get the mutex
    ScopedLock guard(attributeManagerMutex);
    // find the attribute in the map
    AttributeNameInfoMap::iterator attItr = myAttributes.find(attributeLongName);

    // if attribute is found
    if(attItr != myAttributes.end())
    {
        // get attribute info
        AttributeInfoInternal* att = (*attItr).second;

        // create a new SlotID instance with slot of the attribute
        SlotID slotID = att->Slot();


        return slotID;
    }
    else
    {
        // attribute is not found, this may be due to wrong relation or attribute name
        // return an invalid slot
        SlotID invalid;
        return invalid;
        //FATAL("No attribute found with name %s!", attributeLongName.c_str())
    }
}

string AttributeManager::GetAttributeName(SlotID slot){
    string rez; // the result
    //get the mutex
    ScopedLock guard(attributeManagerMutex);

    // find the attribute in the map
    AttributeSlotToNameMap::iterator attItr = reverseMap.find(slot);

    // if attribute is found
    if(attItr != reverseMap.end())
    {
        // get attribute info
        rez = (*attItr).second;

        return rez;
    }
    else
    {
        // attribute is not found, this may be due to wrong relation or attribute name
        FATAL("No attribute found with slot %i!", (int)slot)
    }
}


SlotID AttributeManager::GetAttributeSlot(string tableName, string attributeName)
{
    // create the key for base relation attribute
    string attLongName = REL_ATT_KEY(tableName, attributeName);

    return GetAttributeSlot(attLongName);
}

void AttributeManager::GetAttributesSlots(string tableName, SlotContainer& where){
    //get the mutex
    ScopedLock guard(attributeManagerMutex);

    RelationToSlotsMap::iterator mapItr;

    //find ColumnToSlot mapping for given relation
    mapItr = relationToSlots.find(tableName);

    //if mapping is found
    if(mapItr != relationToSlots.end())
    {
        //copy the mapping information
        SlotToSlotMap& slotMap = *((*mapItr).second);
        slotMap.MoveToStart();
        while (!slotMap.AtEnd()){
            SlotID slot = slotMap.CurrentData();
            where.Append(slot);
            slotMap.Advance();
        }

    }
    else
    {
        FATAL("No map found for relation %s!",tableName.c_str());
    }

    // done with critical region
}

void AttributeManager::AliasAttributesSlots(string tableName, string alias, SlotContainer& where ) {
    // Lock the mutex
    ScopedLock guard(attributeManagerMutex);

    // Does the alias already exist?
    RelationToSlotsMap::iterator aliasItr = relationToSlots.find(alias);

    if( aliasItr != relationToSlots.end() ) {
        // If so, just get the slots and return
        SlotToSlotMap& slotMap = *(aliasItr->second);
        slotMap.MoveToStart();

        while(!slotMap.AtEnd()) {
            SlotID slot = slotMap.CurrentData();
            where.Append(slot);
            slotMap.Advance();
        }
    }
    else {
        // Alias not already created. Make sure that the base relation exists.
        RelationToSlotsMap::iterator baseItr = relationToSlots.find(tableName);

        if( baseItr != relationToSlots.end() ) {
            SlotToSlotMap& baseSlotMap = *(baseItr->second);

            SlotToSlotMap* aliasSlotMap = new SlotToSlotMap();

            FOREACH_EM( colSot, baseSlot, baseSlotMap) {
                string baseName = reverseMap[baseSlot];
                AttributeInfoInternal* baseInfo = myAttributes[baseName];

                // Remove the tableName_ from the name of the attribute
                string baseAttrName = baseInfo->ShortName();

                // Get a new slot ID for the attribute
                int newSlot = NextEmptySlot();
                slotUsage.at(newSlot-firstSynthSlot) = true;

                // Get column # from base attribute info
                SlotID colID = baseInfo->Column();
                int colNo = colID.GetInt();

                // Create a new AttributeInfoInternal for the attribute
                AttributeInfoInternal *aliasInfo = new AttributeInfoInternal(
                        alias, baseAttrName, baseInfo->Type(), baseInfo->JType(), colNo, newSlot );

                // This should already have the alias name prefixed
                string aliasAttrName = aliasInfo->Name();

                // Add the attribute info to the myAttributes map
                myAttributes[aliasAttrName] = aliasInfo;
                reverseMap[aliasInfo->Slot()] = aliasAttrName;

                // Make copies of the column and slot just to be sure we don't
                // swap out the values in aliasInfo
                SlotID tCol = aliasInfo->Column();
                SlotID tSlot = aliasInfo->Slot();
                aliasSlotMap->Insert(tCol, tSlot);

                SlotID sID = aliasInfo->Slot();
                where.Append(sID);

            } END_FOREACH;

            relationToSlots[alias] = aliasSlotMap;
        }
        else {
            FATAL("Unable to create alias for non-existent relation %s.", tableName.c_str());
        }
    }
}

string AttributeManager::GetAttributeType(string longName){
    string rez; /* initially empty */

    ScopedLock guard(attributeManagerMutex);

    // find the attribute in the map
    AttributeNameInfoMap::iterator attItr = myAttributes.find(longName);

    // if attribute is found
    if(attItr != myAttributes.end())
    {
        // get attribute info
        AttributeInfoInternal* att = (*attItr).second;

        // create a new SlotID instance with slot of the attribute
        rez=att->Type();

    }

    return rez;

}

Json::Value AttributeManager::GetAttributeTypeJSON(string longName) {
    Json::Value rez;

    ScopedLock guard(attributeManagerMutex);

    // find the attribute in the map
    AttributeNameInfoMap::iterator attItr = myAttributes.find(longName);

    // if attribute is found
    if(attItr != myAttributes.end())
    {
        // get attribute info
        AttributeInfoInternal* att = (*attItr).second;

        // create a new SlotID instance with slot of the attribute
        rez = att->JType();

    }

    return rez;
}


SlotID AttributeManager::GetAttributeSlot(QueryID id, string attributeName)
{

    QueryManager& qm=QueryManager::GetQueryManager();
    string qName;
    if (!qm.GetQueryName(id,qName)){
        FATAL("Query is not registered!");
    }

    // create the key for synthesized attribute
    string attLongName = SYN_ATT_KEY(qName, attributeName);

    return GetAttributeSlot(attLongName);
}


SlotID AttributeManager::GetAttributeColumn(string tableName, string attributeName){
    string attLongName=REL_ATT_KEY(tableName, attributeName);
    return GetAttributeColumn(attLongName);
}

SlotID AttributeManager::GetAttributeColumn(string attLongName){
    //get the mutex
    ScopedLock guard(attributeManagerMutex);

    // find the attribute in the map
    AttributeNameInfoMap::iterator attItr = myAttributes.find(attLongName);

    // if attribute is found
    if(attItr != myAttributes.end())
    {
        // get attribute info
        AttributeInfoInternal* att = (*attItr).second;

        // create a new SlotID instance with column of the attribute
        SlotID slotID = att->Column();

        return slotID;
    }
    else
    {
        FATAL("No attribute found with name %s!", attLongName.c_str());
    }
}


void AttributeManager::GetColumnToSlotMapping(string tableName, SlotToSlotMap& where) {

    //get the mutex
    ScopedLock guard(attributeManagerMutex);

    RelationToSlotsMap::iterator mapItr;

    //find ColumnToSlot mapping for given relation
    mapItr = relationToSlots.find(tableName);

    //if mapping is found
    if(mapItr != relationToSlots.end())
    {
        //copy the mapping information
        where.copy(*((*mapItr).second));
    }
    else
    {
        FATAL("No map found for relation %s!",tableName.c_str());
    }
}

void AttributeManager::DeleteAttributes(QueryID queryid) {
    //get the mutex
    ScopedLock guard(attributeManagerMutex);

    // find the query id in the map
    QueryIDToAttributesMap::iterator qtoaMapItr;
    qtoaMapItr = queryIdToAttributes.find(queryid);

    // if present, then remove all the attributes of that query from
    // queryIdToAttributes map and also from myAttributes
    if(qtoaMapItr != queryIdToAttributes.end())
    {
        StringContainer& attributes = (*qtoaMapItr).second;
        StringContainer::iterator attribItr;

        // remove all the synthesized attributes of this queryid from myAttributes
        for(attribItr = attributes.begin(); attribItr != attributes.end(); attribItr++)
        {
            string attribName = (*attribItr);
            // get the attribute info
            AttributeInfoInternal* attributeInfo = (*(myAttributes.find(attribName))).second;
            SlotID attribSlot = attributeInfo->Slot();

            // free the slot of deleted attribute
            slotUsage.at(attribSlot -firstSynthSlot) = false;

            // now delete the attribute from global attribute container
            myAttributes.erase(attribName);
        }

        // free memory occupied by query's attribute container
        attributes.clear();
        // delete attributes;
        //attributes = NULL;

        // now remove entry for the query id
        queryIdToAttributes.erase(queryid);
    }
    else
    {
        WARNING("No Attributes present for given query id %d!", queryid.GetInt());
    }

}

void AttributeManager::GenerateJSON( Json::Value & fillMe ) {
    fillMe = Json::Value(Json::arrayValue);

    //get the mutex
    ScopedLock guard(attributeManagerMutex);

    for( auto el : myAttributes ) {
        AttributeInfoInternal * attributeInfo = el.second;
        SlotID slotID = attributeInfo->Slot();

        SlotInfo slotInfo(1);
        slotID.getInfo(slotInfo);
        string id = slotInfo.getIDAsString();

        string name = attributeInfo->Name();

        string type = attributeInfo->Type();
        Json::Value jType = attributeInfo->JType();

        Json::Value jVal(Json::objectValue);
        jVal[J_NAME] = name;
        jVal[J_TYPE] = jType;
        jVal[J_SLOT] = id;

        fillMe.append(jVal);
    }

}
