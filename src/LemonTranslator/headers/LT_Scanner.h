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
#ifndef _LT_SCANNER_H_
#define _LT_SCANNER_H_

#include "LT_Waypoint.h"

#include "AttributeManager.h"

#include <iostream>
#include <utility>
#include <map>

#include <cstring>
#include <cstddef>

class LT_Scanner : public LT_Waypoint {

    public:
        // name of the relation this scanner scans
        std::string relation;

        SlotSet allAttr;

        // Dropped attributes after top to bottom analysis per query
        QueryToSlotSet dropped;

        QueryToSlotSet fromTextLoader;

        // TODO: Possibly try to make multi-query
        // Currently only supports 1 mapping of slots to columns for writing
        // for all queries currently running.
        // Mapping of Slot In Relation -> Desired Output Slot
        SlotToSlotMap storeMap;

        QueryToScannerRangeList filters;

    public:

        LT_Scanner(WayPointID id, std::string relName, SlotSet& atts):
            LT_Waypoint(id),
            relation(relName),
            allAttr(atts),
            dropped(), fromTextLoader(), storeMap(), filters()
    {
        assert(!allAttr.empty());
    }

        virtual WaypointType GetType() {return ScannerWaypoint;}

        virtual void ClearAllDataStructure();

        virtual void DeleteQuery(QueryID query);

        virtual bool AddScanner(QueryIDSet query);

        virtual bool AddWriter(QueryID query, SlotToSlotMap & qStoreMap);

        virtual bool AddScannerRange(QueryID query, int64_t min, int64_t max);

        virtual bool PropagateDown(QueryID query, const SlotSet& atts, SlotSet& rez, QueryExit qe);

        virtual bool PropagateDownTerminating(QueryID query, const SlotSet& atts/*blank*/, SlotSet& result, QueryExit qe);

        virtual bool PropagateUp(QueryToSlotSet& result);

        virtual void GetDroppedQueries(QueryExitContainer& qe);

        virtual void GetQuerExitToSlotMap(QueryExitToSlotsMap& qe);

        virtual bool GetConfig(WayPointConfigureData& where);

        // This will only get queries from text loader
        virtual void ReceiveAttributes(QueryToSlotSet& atts);

        virtual Json::Value GetJson();
};

#endif // _LT_SCANNER_H_
