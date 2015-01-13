//
//  Copyright 2012 Alin Dobra and Christopher Jermaine
//  Copyrighht 2013 Tera Insights
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
#ifndef _LT_TEXT_LOADER_H_
#define _LT_TEXT_LOADER_H_

#include "LT_Waypoint.h"
#include "LT_Scanner.h"

/** the internal translator class for TextLoaders

    We steal as much as possible from file Scanner
*/

class LT_TextLoader : public LT_Scanner {
private:
  Json::Value expression; // info for code generation

  SlotContainer attributesInOrder;

public:

  LT_TextLoader(WayPointID id, SlotSet& atts, SlotContainer& _attributesInOrder, Json::Value& expr):
    LT_Scanner(id, id.getName(),atts), expression(expr){
        attributesInOrder.swap(_attributesInOrder);
    }
  virtual WaypointType GetType() {return TextLoaderWaypoint;}
  virtual Json::Value GetJson();
  //virtual bool PropagateDownScanner(QueryID query, QueryExit qe, const SlotSet& atts);
  virtual bool GetConfig(WayPointConfigureData& where);
  virtual bool PropagateDown(QueryID query, const SlotSet& atts, SlotSet& rez, QueryExit qe);
};



#endif // _LT_TEXT_LOADER_H_
