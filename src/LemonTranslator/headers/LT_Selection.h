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
#ifndef _LT_SELECTION_H_
#define _LT_SELECTION_H_

#include "LT_Waypoint.h"

class LT_Selection : public LT_Waypoint {
private:
    typedef std::vector<WayPointID> StateSourceVec;

    QueryToJson filters; // map from query to filter info
    QueryToJson synthDefs; // map from synthesized attributes to info

    typedef std::map<QueryID, StateSourceVec> QueryToWayPointIDs;
    QueryToWayPointIDs states;

    QueryToSlotSet synthesized;

public:

    LT_Selection(WayPointID id): LT_Waypoint(id)
    {}

    virtual void ClearAllDataStructure();

    virtual WaypointType GetType() {return SelectionWaypoint;}

    virtual bool AddBypass(QueryID query);

    virtual void DeleteQuery(QueryID query);

    virtual bool AddFilter(QueryID query, SlotSet& atts, StateSourceVec& reqStates, Json::Value& expr);

    virtual bool AddSynthesized(QueryID query, SlotID att, SlotSet& atts, Json::Value& expr);

    virtual bool PropagateDown(QueryID query, const SlotSet& atts, SlotSet& result, QueryExit qe);

    virtual bool PropagateUp(QueryToSlotSet& result);

    // get the content of this waypoint as a large JSON object
    virtual Json::Value GetJson();

    virtual bool GetConfig(WayPointConfigureData& where);
}; // class

#endif // _LT_SELECTION_H_
