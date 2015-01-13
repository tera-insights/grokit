//
//  Copyright 2012 Alin Dobra and Christopher Jermaine
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
#include "LT_GIST.h"
#include "AttributeManager.h"
#include "TwoWayList.h"

using namespace std;

bool LT_GIST :: GetConfig(WayPointConfigureData& where) {
    // Get the ID
    WayPointID myID = GetId();

    // Set up work functions.
    GISTPreProcessWorkFunc preprocess(NULL);
    GISTNewRoundWorkFunc newRound(NULL);
    GISTDoStepsWorkFunc doSteps(NULL);
    GISTMergeStatesWorkFunc mergeStates(NULL);
    GISTShouldIterateWorkFunc shouldIterate(NULL);
    GISTProduceResultsWorkFunc produceResults(NULL);
    GISTProduceStateWorkFunc produceState(NULL);

    WorkFuncContainer myWorkFuncs;

    myWorkFuncs.Insert(preprocess);
    myWorkFuncs.Insert(newRound);
    myWorkFuncs.Insert(doSteps);
    myWorkFuncs.Insert(mergeStates);
    myWorkFuncs.Insert(shouldIterate);
    myWorkFuncs.Insert(produceResults);
    myWorkFuncs.Insert(produceState);

    // Manage query exits
    QueryExitContainer myEndingQueryExits;
    QueryExitContainer myFlowThroughQueryExits;
    GetQueryExits (myFlowThroughQueryExits, myEndingQueryExits);

    // Managed required states
    QueryToReqStates myReqStates;
    QueryIDToBool myReturnStates;
    for( auto it : stateSources ) {
        QueryID curID = it.first;
        StateSourceVec& reqStates = it.second;

        ReqStateList stateList;

        for( StateSourceVec::iterator iter = reqStates.begin(); iter != reqStates.end(); ++iter) {
            WayPointID curSource = *iter;
            stateList.Append(curSource);
        }

        myReqStates.Insert( curID, stateList );
    }

    for( auto it : infoMap ) {
        QueryID qID = it.first;
        Json::Value & info = it.second;

        Swapify<bool> retState = info[J_STATE].asBool();
        myReturnStates.Insert(qID, retState);
    }

    // Create the configuration data.
    GISTConfigureData myConfig(myID, myWorkFuncs, myEndingQueryExits,
            myFlowThroughQueryExits, myReqStates, myReturnStates);

    where.swap(myConfig);

    return true;
}

void LT_GIST :: DeleteQuery(QueryID query) {
    DeleteQueryCommon(query);
    synthesized.erase(query);
    QueryID qID;
    SlotContainer slotCont;

    infoMap.erase(query);
}

void LT_GIST :: ClearAllDataStructure() {
    ClearAll();
    synthesized.clear();
    infoMap.clear();
}

bool LT_GIST :: AddGIST( QueryID query,
			 SlotContainer& resultAtts,
             StateSourceVec & sVec,
			 Json::Value& info) {
    infoMap[query] = info;
    stateSources[query] = sVec;

    SlotSet rAtts;
    SlotContainerToSet(resultAtts, rAtts);

    queriesCovered.Union(query);
    CheckQueryAndUpdate(query, rAtts, synthesized);

    return true;
}

// atts coming from top is dropped as it will not be propagated down
// that also means, all attributes coming from top are synthesized ones
// 1. result is blank, as this waypoint uses no attributes
// 2. Correctness : Atts coming from top is subset of synthesized
bool LT_GIST :: PropagateDown(QueryID query, const SlotSet& atts, SlotSet& result, QueryExit qe) {
    // atts coming from top should be subset of synthesized
    if( !IsSubSet(atts, synthesized[query]) ) {
        cerr << "GISTWP : Aggregate error: attributes coming from top are not subset of synthesized ones" << endl;
        cerr << "PropgateDown for Waypoint " << GetWPName() << endl;
        cerr << "Query: " << query.ToString() << endl;
        /* FIXME: GetAllAttrAsString got removed somehow
        cerr << "a0tts: " << GetAllAttrAsString(atts) << endl;
        cerr << "synthesized[query]: " << GetAllAttrAsString(synthesized[query]) << endl;
        */
        return false;
    }

    result.clear();
    queryExit.Insert(qe);
    return true;
}

bool LT_GIST :: PropagateDownTerminating(QueryID query, const SlotSet& atts, SlotSet& result, QueryExit qe ) {
    // atts coming from top should be subset of synthesized
    if( !IsSubSet(atts, synthesized[query]) ) {
        cerr << "GISTWP : Aggregate error: attributes coming from top are not subset of synthesized ones" << endl;
        cerr << "PropgateDownTerminating for Waypoint " << GetWPName() << endl;
        cerr << "Query: " << query.ToString() << endl;
        /* FIXME: GetAllAttrAsString got removed somehow
        cerr << "atts: " << GetAllAttrAsString(atts) << endl;
        cerr << "synthesized[query]: " << GetAllAttrAsString(synthesized[query]) << endl;
        */
        return false;
    }

    queryExitTerminating.Insert(qe);
    return true;
}

// GIST uses no attributes, so just say that our result is the attributes we're producing.
bool LT_GIST :: PropagateUp(QueryToSlotSet& result) {
    result.clear();
    result = synthesized;

    return true;
}

Json::Value LT_GIST::GetJson(){
  Json::Value out(Json::objectValue);// overall object to be return

  IDInfo info;
  GetId().getInfo(info);
  out[J_NAME] = info.getName();
  out[J_TYPE] = JN_GIST_WP;

  out[J_ARGS] = MapToJson(infoMap);

  SlotToQuerySet reverse;
  AttributesToQuerySet(used, reverse);
  out[J_ATT_MAP] = JsonAttToQuerySets(reverse);

  return out;
}
