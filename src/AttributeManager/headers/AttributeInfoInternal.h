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
#ifndef _ATTRIBUTE_INFO_INTERNAL_
#define _ATTRIBUTE_INFO_INTERNAL_

#include <string>

#include "AttributeType.h"
#include "ID.h"
#include "JsonAST.h"

#define REL_ATT_KEY(relationName, attName) relationName + "_" + attName
#define SYN_ATT_KEY(qName, attName) qName + "_" + attName

/** This class holds details of the attribute including its name, and slot information.
 * This class is used privately by AttributeManager.
 **/
class AttributeInfoInternal {
    private:
        // attribute name
        std::string name;

        // base attribute name without table or query prefix
        std::string shortName;

        // relation to which attribute belongs
        std::string relationName;

        // type of the attribute
        std::string type;

        Json::Value jType;

        // attribute slot
        SlotID slot;

        // attribute column
        SlotID column;

    public:
        // constructor for attributes from base relations
        AttributeInfoInternal(std::string _relationName, std::string _name, std::string _type, Json::Value _jType,
                int _column, int _slot = -1);

        // constructor for synthesized attributes
        AttributeInfoInternal(QueryID& id, std::string _name, std::string _type, Json::Value _jType, int _slot = -1);

        // destructor
        virtual ~AttributeInfoInternal() {}

        // setter method of slot
        void SetSlot(SlotID _slot){ slot=_slot; }

        // setter method for column
        void SetColumn(SlotID _column){ column = _column; }

        // access methods
        std::string ShortName(void) { return shortName; }
        std::string Name(void){ return name;}
        std::string Type(void){ return type;}

        Json::Value JType(void) { return jType; }

        SlotID Slot(void){ return slot; }

        SlotID Column(void){ return column; }
};

#endif // _ATTRIBUTE_INFO_INTERNAL_
