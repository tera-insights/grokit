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
#ifndef _LT_JOIN_H_
#define _LT_JOIN_H_

#include "LT_Waypoint.h"
#include "EfficientMap.h"

class LT_Join : public LT_Waypoint {
private:


    // Common attributes LHS
    SlotSet LHS_atts;
    // Common attributes LHS, in join order
    SlotVec LHS_keys;

    // Data received from terminting edge during bottom up analysis to be used in top down later
    QueryToSlotSet RHS_terminating;
    // Data for RHS while adding query attributes pairs
    QueryToSlotSet RHS;
    // The keys (join attributes) per query for RHS
    // Must be slot containers to retain proper ordering
    QueryToSlotVec RHS_keys;
    // attributes to copy for each query
    QueryToSlotSet LHS_copy;
    QueryToSlotSet RHS_copy;

    QueryIDSet ExistsTarget; // set of queries for which we run an exists predicate
    QueryIDSet NotExistsTarget; // same for notExists

    // id of cleaner so we can write config messages
    WayPointID cleanerID;

    // Definitions required for all queries
    std::string global_defs;

    Json::Value global_info;

    // Definitions required per query
    typedef std::map< QueryID, std::string > QueryIDToString;
    QueryIDToString query_defs;

    QueryToJson per_query_info;

public:

    LT_Join(WayPointID id, const SlotSet& atts, const SlotVec& atts_keys, WayPointID _cleanerID, Json::Value& info):
        LT_Waypoint(id),
        LHS_atts(atts),
        LHS_keys(atts_keys),
        RHS_terminating(),
        RHS(),
        RHS_keys(),
        LHS_copy(),
        RHS_copy(),
        ExistsTarget(),
        NotExistsTarget(),
        cleanerID(_cleanerID),
        global_defs(info[J_C_DEFS].asString()),
        global_info(info),
        query_defs(),
        per_query_info()
    { }

    virtual WaypointType GetType() {return JoinWaypoint;}

    virtual void ClearAllDataStructure();

    virtual void ReceiveAttributesTerminating(QueryToSlotSet& atts);

    virtual bool AddBypass(QueryID query);

    virtual void DeleteQuery(QueryID query);

    virtual bool AddJoin(QueryID query, SlotSet& RHS_atts, SlotVec& keys, LemonTranslator::JoinType jType, Json::Value& info);

    // Consider this as LHS
    virtual bool PropagateDown(QueryID query, const SlotSet& atts, SlotSet& result, QueryExit qe);

    virtual bool PropagateDownTerminating(QueryID query, const SlotSet& atts, SlotSet& result, QueryExit qe);

    virtual bool PropagateUp(QueryToSlotSet& result);

    virtual bool GetConfig(WayPointConfigureData& where);

    virtual bool GetConfigs(WayPointConfigurationList& where);

    virtual void GetAccumulatedLHSRHS(std::set<SlotID>& LHS, std::set<SlotID>& RHS, QueryIDSet& queries);
    virtual void GetAccumulatedLHSRHSAtts(std::set<SlotID>& LHS, std::set<SlotID>& RHS);

    virtual void GetQueryExitToSlotMapLHS(QueryExitToSlotsMap& qe);
    virtual void GetQueryExitToSlotMapRHS(QueryExitToSlotsMap& qe);

    virtual Json::Value GetJson();
};


#endif // _LT_JOIN_H_
