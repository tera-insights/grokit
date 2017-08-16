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

#include <algorithm>

#include <boost/algorithm/string.hpp>

#include "LT_Selection.h"
#include "AttributeManager.h"
#include "Errors.h"
#include "JsonAST.h"

using namespace std;

bool LT_Selection::GetConfig(WayPointConfigureData& where){

  // get the ID
  WayPointID selectionID = GetId ();

  // first, get the function we will send to it
  WorkFunc tempFunc = NULL; // NULL, will be populated in codeloader
  SelectionPreProcessWorkFunc myPreProcessWorkFunc(tempFunc);
  SelectionProcessChunkWorkFunc mySelectionWorkFunc (tempFunc);
  WorkFuncContainer mySelectionWorkFuncs;
  mySelectionWorkFuncs.Insert(myPreProcessWorkFunc);
  mySelectionWorkFuncs.Insert (mySelectionWorkFunc);

  // this is the set of query exits that end at it, and flow through it
  QueryExitContainer mySelectionEndingQueryExits;
  QueryExitContainer mySelectionFlowThroughQueryExits;
  GetQueryExits (mySelectionFlowThroughQueryExits, mySelectionEndingQueryExits);
  PDEBUG("Printing query exits for SELECTION WP ID = %s", selectionID.getName().c_str());
#ifdef DEBUG
        cout << "\nFlow through query exits\n" << flush;
        mySelectionFlowThroughQueryExits.MoveToStart();
        while (mySelectionFlowThroughQueryExits.RightLength()) {
                (mySelectionFlowThroughQueryExits.Current()).Print();
                mySelectionFlowThroughQueryExits.Advance();
        }
        cout << "\nEnding query exits\n" << flush;
        mySelectionEndingQueryExits.MoveToStart();
        while (mySelectionEndingQueryExits.RightLength()) {
                (mySelectionEndingQueryExits.Current()).Print();
                mySelectionEndingQueryExits.Advance();
        }
        cout << endl;
#endif

  QueryToReqStates myReqStates;
  for( QueryToWayPointIDs::iterator it = states.begin(); it != states.end(); ++it ) {
    QueryID curID = it->first;
    StateSourceVec & reqStates = it->second;

    ReqStateList stateList;

    for( StateSourceVec::iterator iter = reqStates.begin(); iter != reqStates.end(); ++iter  ) {
      WayPointID curSource = *iter;
      stateList.Append(curSource);
    }

    myReqStates.Insert( curID, stateList );
  }

  myReqStates.MoveToStart();
  FATALIF(myReqStates.AtEnd(), "Query to Required States Mapping empty for Selection WayPoint!");

  // here is the waypoint configuration data
  SelectionConfigureData selectionConfigure (selectionID, mySelectionWorkFuncs, mySelectionEndingQueryExits, mySelectionFlowThroughQueryExits, myReqStates);

  where.swap (selectionConfigure);

  return true;
}

bool LT_Selection::AddBypass(QueryID query) {
  bypassQueries.Union(query);
  queriesCovered.Union(query);
  return true;
}

void LT_Selection::DeleteQuery(QueryID query) {
  DeleteQueryCommon(query);
  filters.erase(query);
}

void LT_Selection::ClearAllDataStructure() {
  ClearAll();
  filters.clear();
  synthesized.clear();
}

bool LT_Selection::AddFilter(QueryID query, SlotSet& atts, StateSourceVec& reqStates, Json::Value& expr) {
  CheckQueryAndUpdate(query, atts, newQueryToSlotSetMap);

  // we want to deal with the situation in which the predicate is specified as a series
  // of independent predicates

  filters[query]=expr;

  if( synthDefs.find(query) == synthDefs.end() ) {
      synthDefs[query] = Json::Value(Json::arrayValue);
  }

  states[query] = reqStates;

  queriesCovered.Union(query);
  return true;
}

bool LT_Selection::AddSynthesized(QueryID query, SlotID att,
        SlotSet& atts, Json::Value& expr) {

    if( synthDefs.find(query) == synthDefs.end() )
        synthDefs[query] = Json::Value(Json::arrayValue);
        synthDefs[query].append(expr);
        CheckQueryAndUpdate(query, atts, newQueryToSlotSetMap);
        queriesCovered.Union(query);

    // if a filter for this query does not exist, must add a "true" filter
    if (filters.find(query) == filters.end()){
        Json::Value tmp(Json::objectValue);
        tmp[J_ARGS] = Json::Value(Json::arrayValue);
        tmp[J_TYPE] = Json::Value(Json::nullValue);
        tmp[J_SARGS] = Json::Value(Json::arrayValue);
        tmp[J_CARGS] = Json::Value(Json::arrayValue);

        filters[query] = tmp;
        StateSourceVec emptyVec;
        states[query] = emptyVec;
    }

    //synthesized
    SlotSet attsSet;
    attsSet.insert(att);
    CheckQueryAndUpdate(query, attsSet, synthesized);

    return true;
}

// Implementation top -> down as follows per query:
// 1. used = used + new queries attributes filled after analysis
// 2. result = used + attributes coming from above - synthesized
//    (synthesized dont need to go down)
bool LT_Selection::PropagateDown(QueryID query, const SlotSet& atts, SlotSet& result, QueryExit qe) {
  CheckQueryAndUpdate(newQueryToSlotSetMap, used);
  newQueryToSlotSetMap.clear();
  result.clear();
  SlotSet uni;
  set_union(atts.begin(), atts.end(), used[query].begin(), used[query].end(), inserter(uni, uni.begin()));
  set_difference(uni.begin(), uni.end(), synthesized[query].begin(), synthesized[query].end(), inserter(result, result.begin()));
  queryExit.Insert (qe);

    std::cerr << "[Selection] " << GetWPName() << " query " << GetQueryName(query) << ":\n"
        << "\tNeeded By Next WP: " << GetAllAttrAsString(atts) << "\n"
        << "\tNeeded By Self: " << GetAllAttrAsString(used[query]) << "\n"
        << "\tSynthesized: " << GetAllAttrAsString(synthesized[query]) << "\n"
        << "\tRequested From Below: " << GetAllAttrAsString(result) << "\n";

  return true;
}

// Implementation bottom -> up as follows for all queries together:
// 1. used = used + new queries attributes filled after analysis
// 2. result = attributes coming from below + synthesized
// 3. Dont need to add new attributes received since last analysis to the result
// 4. Correctness : used is subset of what is coming from below
bool LT_Selection::PropagateUp(QueryToSlotSet& result) {
  CheckQueryAndUpdate(newQueryToSlotSetMap, used);
  newQueryToSlotSetMap.clear();
  result.clear();

  // result = attributes coming from below + synthesized
  CheckQueryAndUpdate(downAttributes, result);
  CheckQueryAndUpdate(synthesized, result);

  //PrintAllQueryAndAttributes(used);
  //PrintAllQueryAndAttributes(downAttributes);

  // Correctness
  // used should be subset of what is coming from below
  if (!IsSubSet(used, downAttributes))
    {
      cout << "Selection WP : Attribute mismatch : used is not subset of attributes coming from below\n";
      return false;
    }
  downAttributes.clear();
  return true;
}

Json::Value LT_Selection::GetJson(){
    Json::Value out(Json::objectValue);// overall object to be return

    IDInfo info;
    GetId().getInfo(info);
    out[J_NAME] = info.getName();
    out[J_TYPE] = JN_SEL_WP;

    AttributeManager& am = AttributeManager::GetAttributeManager();

    out[J_FILTERS] = MapToJson(filters);
    out[J_SYNTH] = MapToJson(synthDefs);

    // print the Attributes with QueryIDSets in which they are used
    // format: (att_name, QueryIDSet_serialized), ..
    SlotToQuerySet reverse;
    AttributesToQuerySet(used, reverse);
    out[J_ATT_MAP] = JsonAttToQuerySets(reverse);

    return out;
}
