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
#include "LT_GLA.h"
#include "AttributeManager.h"
#include <boost/algorithm/string.hpp>

using namespace std;

bool LT_GLA::GetConfig(WayPointConfigureData& where){

    // get the ID
    WayPointID glaIDOne = GetId ();

    // first, get the function we will send to it
    //    WorkFunc tempFunc = NULL;
    GLAPreProcessWorkFunc GLAPreProcessWF(NULL);
    GLAProcessChunkWorkFunc GLAProcessChunkWF (NULL);
    GLAMergeStatesWorkFunc GLAMergeWF (NULL);
    GLAPreFinalizeWorkFunc GLAPreFinalizeWF(NULL);
    GLAFinalizeWorkFunc GLAFinalizeWF (NULL);
    GLAFinalizeStateWorkFunc GLAFinalizeStateWF(NULL);
    GLAPostFinalizeWorkFunc GLAPostFinalizeWF(NULL);
    WorkFuncContainer myGLAWorkFuncs;
    myGLAWorkFuncs.Insert (GLAPreProcessWF);
    myGLAWorkFuncs.Insert (GLAProcessChunkWF);
    myGLAWorkFuncs.Insert (GLAMergeWF);
    myGLAWorkFuncs.Insert (GLAPreFinalizeWF);
    myGLAWorkFuncs.Insert (GLAFinalizeWF);
    myGLAWorkFuncs.Insert (GLAFinalizeStateWF);
    myGLAWorkFuncs.Insert (GLAPostFinalizeWF);

    // this is the set of query exits that end at it, and flow through it
    QueryExitContainer myGLAEndingQueryExits;
    QueryExitContainer myGLAFlowThroughQueryExits;
    GetQueryExits (myGLAFlowThroughQueryExits, myGLAEndingQueryExits);
    PDEBUG("Printing query exits for AGGREGATE WP ID = %s", glaIDOne.getName().c_str());
#ifdef DEBUG
        cout << "\nFlow through query exits\n" << flush;
        myGLAFlowThroughQueryExits.MoveToStart();
        while (myGLAFlowThroughQueryExits.RightLength()) {
                (myGLAFlowThroughQueryExits.Current()).Print();
                myGLAFlowThroughQueryExits.Advance();
        }
        cout << "\nEnding query exits\n" << flush;
        myGLAEndingQueryExits.MoveToStart();
        while (myGLAEndingQueryExits.RightLength()) {
                (myGLAEndingQueryExits.Current()).Print();
                myGLAEndingQueryExits.Advance();
        }
        cout << endl;
#endif

    QueryToReqStates myReqStates;
    for( auto it = stateSources.begin(); it != stateSources.end(); ++it ) {
        QueryID curID = it->first;
        StateSourceVec & reqStates = it->second;

        ReqStateList stateList;

        for( StateSourceVec::iterator iter = reqStates.begin(); iter != reqStates.end(); ++iter  ) {
            WayPointID curSource = *iter;
            stateList.Append(curSource);
        }

        myReqStates.Insert( curID, stateList );
    }

    QueryIDToBool myReturnStates;
    for( auto it : infoMap ) {
        QueryID qID = it.first;
        Json::Value & info = it.second;

        Swapify<bool> retState = info[J_STATE].asBool();
        myReturnStates.Insert(qID, retState);
    }

    myReqStates.MoveToStart();
    FATALIF(myReqStates.AtEnd(), "There should be something in this map");

    // here is the waypoint configuration data
    GLAConfigureData glaConfigure (glaIDOne, myGLAWorkFuncs,  myGLAEndingQueryExits, myGLAFlowThroughQueryExits,
            myReqStates, myReturnStates);

    // and add it
    where.swap (glaConfigure);

    return true;
}

void LT_GLA::DeleteQuery(QueryID query)
{
    DeleteQueryCommon(query); // common data
    synthesized.erase(query);
    QueryID qID;
    SlotContainer slotCont;

    infoMap.erase(query);
}

void LT_GLA::ClearAllDataStructure() {
    ClearAll(); // common data
    infoMap.clear();
    synthesized.clear();
}

bool LT_GLA::AddGLA(QueryID query,
        SlotContainer& resultAtts,
        SlotSet& atts,
        StateSourceVec& sVec,
        Json::Value& info)
{
    infoMap[query] = info;
    stateSources[query] = sVec;
    CheckQueryAndUpdate(query, atts, newQueryToSlotSetMap);
    queriesCovered.Union(query);
    //synthesized
    SlotSet rAtts;
    SlotContainerToSet(resultAtts, rAtts);
    CheckQueryAndUpdate(query, rAtts, synthesized);

    return true;
}

bool LT_GLA::ReturnAsState( QueryID query ) {
    Json::Value & info = infoMap[query];

    info[J_STATE] = true;
    return true;
}

// atts coming from top is dropped as it will not be propagated down
// that also means, all attributes coming from top are synthesized ones
// 1. used = used + new queries attributes filled after analysis
// 2. result goes down = used
// 3. Correctness : Atts coming from top is subset of synthesized
bool LT_GLA::PropagateDown(QueryID query, const SlotSet& atts, SlotSet& result, QueryExit qe)
{
    // atts coming from top should be subset of synthesized.
    if (!IsSubSet(atts, synthesized[query]))
    {
        cerr << "GLAWP : Aggregate error: attributes coming from top are not subset of synthesized ones" << endl;
        cerr << "PropgateDown for Waypoint " << GetWPName() << endl;
        cerr << "Query: " << query.ToString() << endl;
        /* FIXME: GetAllAttrAsString not defined for some reason
        cerr << "atts: " << GetAllAttrAsString(atts) << endl;
        cerr << "synthesized[query]: " << GetAllAttrAsString(synthesized[query]) << endl;
        */
        return false;
    }

    CheckQueryAndUpdate(newQueryToSlotSetMap, used);
    result.clear();
    result = used[query];
    queryExit.Insert (qe);
    return true;
}

bool LT_GLA::PropagateDownTerminating(QueryID query, const SlotSet& atts, SlotSet& result, QueryExit qe) {
    // atts coming from top should be subset of synthesized.
    if (!IsSubSet(atts, synthesized[query]))
    {
        cerr << "GLAWP : Aggregate error: attributes coming from top are not subset of synthesized ones" << endl;
        cerr << "PropgateDownTerminating for Waypoint " << GetWPName() << endl;
        cerr << "Query: " << query.ToString() << endl;
        /* FIXME: GetAllAttrAsString not defined for some reason
        cerr << "atts: " << GetAllAttrAsString(atts) << endl;
        cerr << "synthesized[query]: " << GetAllAttrAsString(synthesized[query]) << endl;
        */
        return false;
    }

    //CheckQueryAndUpdate(newQueryToSlotSetMap, used);
    //result.clear();
    //result = used[query];
    queryExitTerminating.Insert(qe);
    return true;
}

// Implementation bottom -> up as follows for all queries together:
// 1. used = used + new queries attributes added since last analysis
// 2. clear the new data
// 3. result = NONE
// 4. Print is last destination hence result is blank
// 5. old used + new = used is good to check correctness if they are subset of down attributes
bool LT_GLA::PropagateUp(QueryToSlotSet& result)
{
    CheckQueryAndUpdate(newQueryToSlotSetMap, used);
    newQueryToSlotSetMap.clear();

    result.clear();
    result = synthesized;

    // used should be subset of what is coming from below
    if (!IsSubSet(used, downAttributes))
    {
        cout <<  "GLAWP : Attribute mismatch : used is not subset of attributes coming from below\n";
        return false;
    }
    return true;
}

Json::Value LT_GLA::GetJson(){
  Json::Value out(Json::objectValue);// overall object to be return

  IDInfo info;
  GetId().getInfo(info);
  out[J_NAME] = info.getName();
  out[J_TYPE] = JN_GLA_WP;

  out[J_PAYLOAD] = MapToJson(infoMap);

  SlotToQuerySet reverse;
  AttributesToQuerySet(used, reverse);
  out[J_ATT_MAP] = JsonAttToQuerySets(reverse);

  return out;
}

