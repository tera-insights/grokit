//
//  Copyright 2013 Christopher Dudley
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

#include "LT_GI.h"
#include "AttributeManager.h"
#include "WayPointConfigureData.h"
#include "ContainerTypes.h"
#include "Errors.h"
#include "Stl.h"
#include <boost/algorithm/string.hpp>

using namespace std;

bool LT_GI :: GetConfig( WayPointConfigureData& where ) {
  // all query exit pairs
  QueryExitContainer queryExits;

  QueryExitToSlotsMap queryColumnsMap;
  GetQuerExitToSlotMap(queryColumnsMap);

  WayPointID tableID = GetId();

  WorkFuncContainer myWorkFuncs;
  WorkFunc tempFunc = NULL;
  GIProduceChunkWorkFunc myWorkFunc(tempFunc);
  myWorkFuncs.Insert(myWorkFunc);

  QueryExitContainer myEndingQueryExits;
  QueryExitContainer myFlowThroughQueryExits;
  GetQueryExits(queryExits, myEndingQueryExits);

  StringContainer files;

  Json::Value jFiles = expression[J_FILE];

  for( auto f : jFiles ) {
    files.push_back(f.asString());
  }

  GIConfigureData giConfig( GetId(), myWorkFuncs, myEndingQueryExits,
          myFlowThroughQueryExits, files, queryExits );

  giConfig.swap(where);


  return true;
}

bool LT_GI::PropagateDown(QueryID query, const SlotSet& atts, SlotSet& rez, QueryExit qe)
{
  CheckQueryAndUpdate(newQueryToSlotSetMap, used);
  newQueryToSlotSetMap.clear();

  if (!IsSubSet(atts, used[query]))
    {
      cout << "GI WP : Attributes coming from above should be subset of used\n";
      return false;
    }
  // nothing to be done for rez (result) because we have no-one to propagate down, we are bottommost
  queryExit.Insert(qe);
  return true;
}

Json::Value LT_GI::GetJson(){
  Json::Value out(Json::objectValue);// overall object to be return

  IDInfo info;
  GetId().getInfo(info);
  out[J_NAME] = info.getName();
  out[J_TYPE] = JN_GI_WP;
  out[J_PAYLOAD] = expression;
  out[J_QUERIES] = JsonQuerySet(queriesCovered);

  SlotToQuerySet reverse;
  AttributesToQuerySet(used, reverse);
  out[J_ATT_MAP] = JsonAttToQuerySets(reverse);

  return out;
}
