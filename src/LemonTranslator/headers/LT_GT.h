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
#ifndef _LT_GT_H_
#define _LT_GT_H_

#include "LT_Waypoint.h"
#include "GLAData.h"

class LT_GT : public LT_Waypoint {

private:

  QueryToJson infoMap; // map from query to expressions defining tranformations

  typedef std::vector<WayPointID> StateSourceVec;
  typedef std::map<QueryID, StateSourceVec> QueryToState;

  QueryToState stateSources;

  // map from query to output attributes
  typedef EfficientMap<QueryID, SlotContainer> QueryToSlotContainer;
  QueryToSlotContainer gfAttribs;

  QueryToSlotSet synthesized;

  // Attributes to pass through the GT, per query
  QueryToSlotSet passThrough;
  // All attributes needed as input for the GT, passthrough, or both
  QueryToSlotSet inputNeeded;

public:

    LT_GT(WayPointID id):
        LT_Waypoint(id),
        infoMap(),
        stateSources(),
        gfAttribs(),
        synthesized(),
        passThrough(),
        inputNeeded()
    {}

    virtual WaypointType GetType() {return GTWaypoint;}

    virtual void DeleteQuery(QueryID query) override;

    virtual void ClearAllDataStructure() override;

    //GT, one per query basis
    virtual bool AddGT(QueryID query,
            SlotContainer& resultAtts,
            SlotSet& atts,
            StateSourceVec & sVec,
            Json::Value& expr
            ) override;

    virtual bool PropagateDown(QueryID query, const SlotSet& atts, SlotSet& result, QueryExit qe) override;

    virtual bool PropagateDownTerminating(QueryID query, const SlotSet& atts/*blank*/, SlotSet& result, QueryExit qe) override;

    virtual bool PropagateUp(QueryToSlotSet& result) override;

    virtual Json::Value GetJson() override;

    virtual bool GetConfig(WayPointConfigureData& where) override;

private:
    Json::Value QueryToSlotSetJson( const QueryToSlotSet & );

};

#endif // _LT_GT_H_
