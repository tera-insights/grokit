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
#ifndef _LT_GIST_H_
#define _LT_GIST_H_

#include "LT_Waypoint.h"
#include "GLAData.h"

class LT_GIST : public LT_Waypoint {

  typedef std::vector<WayPointID> StateSourceVec;
  typedef std::map<QueryID, StateSourceVec> QueryToState;

  // info for each GIST
  QueryToJson infoMap;
  QueryToState stateSources;

  QueryToSlotSet synthesized;

    public:

  LT_GIST(WayPointID id) : LT_Waypoint(id)
    {}

  virtual WaypointType GetType() override {return GISTWayPoint;}
  virtual void DeleteQuery(QueryID query) override;
  virtual void ClearAllDataStructure() override;

  // GIST, one per query basis
  virtual bool AddGIST(QueryID query,
          SlotContainer& resultAtts,
          StateSourceVec& sVec,
          Json::Value& info) override;

  virtual bool PropagateDown(QueryID query, const SlotSet& atts, SlotSet& result, QueryExit qe) override;
  virtual bool PropagateDownTerminating(QueryID query, const SlotSet& atts /*blank*/, SlotSet& result,
          QueryExit qe) override;
  virtual bool PropagateUp(QueryToSlotSet& result) override;
  virtual Json::Value GetJson() override;
  virtual bool GetConfig(WayPointConfigureData& where) override;
};

#endif // _LT_GIST_H_
