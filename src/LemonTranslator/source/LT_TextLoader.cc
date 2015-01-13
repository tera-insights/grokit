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
#include "LT_TextLoader.h"
#include "AttributeManager.h"
#include "WayPointConfigureData.h"
#include "ContainerTypes.h"
#include <boost/algorithm/string.hpp>

using namespace std;

bool LT_TextLoader :: GetConfig (WayPointConfigureData& where) {

    // Name of waypoint
    string relation = GetWPName();

    // all query exit pairs
    QueryExitContainer queryExits;

    // dropped queries
    QueryExitContainer qExitsDone;
    // Alin: not working properly. Returns current queries          GetDroppedQueries(qExitsDone);

    // query to slot map
    QueryExitToSlotsMap queryColumnsMap;
    GetQuerExitToSlotMap(queryColumnsMap);

    WayPointID tableID = GetId ();
    /* crap from common inheritance from waypoint*/
    WorkFuncContainer myWorkFuncs;
    WorkFunc tempFunc = NULL; // NULL, will be populated in codeloader
    TextLoaderWorkFunc myWorkFunc (tempFunc);
    myWorkFuncs.Insert(myWorkFunc);

    QueryExitContainer myEndingQueryExits;
    QueryExitContainer myFlowThroughQueryExits;
    GetQueryExits(queryExits, myEndingQueryExits);

    PDEBUG("Printing query exits for TEXTLOADER WP ID = %s", GetId().getName().c_str());

    // scan the json to
    StringContainer filesCont;
    Json::Value files = expression[J_FILE];
    for (int i=0; i<files.size(); ++i)
        filesCont.push_back (files[i].asString());

    TextLoaderConfigureData loaderConfig( GetId(), myWorkFuncs, myEndingQueryExits,
            myFlowThroughQueryExits, filesCont, queryExits);

    where.swap(loaderConfig);

    return true;

}

bool LT_TextLoader::PropagateDown(QueryID query, const SlotSet& atts, SlotSet& rez, QueryExit qe)
{
    CheckQueryAndUpdate(newQueryToSlotSetMap, used);
    newQueryToSlotSetMap.clear();

    if (!IsSubSet(atts, used[query]))
    {
        cout << "TextLoader WP : Attributes coming from above should be subset of used\n";
        return false;
    }
    // nothing to be done for rez (result) because we have no-one to propagate down, we are bottommost
    queryExit.Insert(qe);
    return true;
}

Json::Value LT_TextLoader :: GetJson(){
    Json::Value out(Json::objectValue);// overall object to be return

    IDInfo info;
    GetId().getInfo(info);
    out[J_NAME] = info.getName();
    out[J_TYPE] = JN_TL_WP;
    out[J_EXPR] = expression;

    return out;
}



