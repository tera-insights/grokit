//
//  Copyright 2013 Tera Insights
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

// QUESTION: WHY IS THIS NOT  MULTI-QUERY?  Alin 04/30/2013

#ifndef _LT_GI_H_
#define _LT_GI_H_

#include "LT_Waypoint.h"
#include "LT_Scanner.h"
#include "ContainerTypes.h"

class LT_GI : public LT_Scanner {

    // GI info
    Json::Value expression; // the information maintained for the GI

public:
 LT_GI(WayPointID _id, SlotSet& _attributes, Json::Value& expr ) :
   LT_Scanner(_id, _id.getName(), _attributes), expression(expr){
   }

  virtual WaypointType GetType() { return GIWayPoint; }
  virtual bool GetConfig(WayPointConfigureData& where);
  virtual Json::Value GetJson();
  virtual bool PropagateDown(QueryID query, const SlotSet& atts, SlotSet& rez, QueryExit qe);
};

#endif // _LT_GI_H_
