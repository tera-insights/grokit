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

private:
	typedef std::map<std::string, AttributeInfoInternal*> AttributeNameInfoMap;
    typedef std::map<SlotID, std::string> AttributeSlotToNameMap;
	typedef std::map<std::string, SlotToSlotMap*> RelationToSlotsMap;
	typedef std::map<QueryID, StringContainer> QueryIDToAttributesMap;
	typedef std::vector<bool> Slots;

	// Mutex for the AttributeManager
	pthread_mutex_t attributeManagerMutex;

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
