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
#ifndef _LT_PRINT_H_
#define _LT_PRINT_H_

#include "LT_Waypoint.h"


class LT_Print : public LT_Waypoint {
private:

    QueryToJson infoMap;

public:

    LT_Print(WayPointID id): LT_Waypoint(id)
    {}

    virtual WaypointType GetType() {return PrintWaypoint;}

    virtual void ClearAllDataStructure();

    virtual void DeleteQuery(QueryID query);

    virtual bool AddPrint(QueryID query, SlotSet& atts, Json::Value& info);

    virtual bool PropagateDownTerminating(QueryID query, const SlotSet& atts/*blank*/, SlotSet& result, QueryExit qe);

    virtual bool PropagateUp(QueryToSlotSet& result);

    virtual void ReceiveAttributesTerminating(QueryToSlotSet& atts);

    virtual Json::Value GetJson();

    virtual bool GetConfig(WayPointConfigureData& where);
};

#endif // _LT_PRINT_H_
