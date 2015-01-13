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
#ifndef _LT_GLA_H_
#define _LT_GLA_H_

#include "LT_Waypoint.h"
#include "GLAData.h"

class LT_GLA : public LT_Waypoint {

    private:

        typedef std::vector<WayPointID> StateSourceVec;
        typedef std::map<QueryID, StateSourceVec> QueryToState;

        QueryToState stateSources;

        // info for each GLA attribute-container
        QueryToJson infoMap;

        QueryToSlotSet synthesized;

    public:

        LT_GLA(WayPointID id):
            LT_Waypoint(id),
            stateSources(),
            infoMap(),
            synthesized()
        { }

        virtual WaypointType GetType() {return GLAWaypoint;}
        virtual void DeleteQuery(QueryID query);
        virtual void ClearAllDataStructure();

        //GLA, one per query basis
        virtual bool AddGLA(QueryID query,
                SlotContainer& resultAtts,
                SlotSet& atts,
                StateSourceVec & sVec,
                Json::Value& info);

        virtual bool ReturnAsState(QueryID query);

        virtual bool PropagateDown(QueryID query, const SlotSet& atts, SlotSet& result, QueryExit qe);
        virtual bool PropagateDownTerminating(QueryID query, const SlotSet& atts/*blank*/, SlotSet& result, QueryExit qe);
        virtual bool PropagateUp(QueryToSlotSet& result);
        virtual Json::Value GetJson();
        virtual bool GetConfig(WayPointConfigureData& where);
};

#endif // _LT_GLA_H_
