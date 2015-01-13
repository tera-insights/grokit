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

#include "LT_GT.h"
#include "AttributeManager.h"
#include "json.h"

using namespace std;

namespace {

string GetAllAttrAsString(const set<SlotID>& atts) {
    AttributeManager& am = AttributeManager::GetAttributeManager();
    string rez;
    bool first = true;
    for (set<SlotID>::iterator it = atts.begin(); it != atts.end(); it++) {
        if (first)
            first = false;
        else
            rez += ", ";

        SlotID slot = *it;
        rez += am.GetAttributeName(slot);
    }
    return rez;
}

} // anonymous namespace

Json::Value LT_GT :: QueryToSlotSetJson( const QueryToSlotSet & val ) {
    AttributeManager& am = AttributeManager::GetAttributeManager();

    Json::Value ret(Json::objectValue);

    for( const auto & elem : val ) {
        QueryID qry = elem.first;
        const SlotSet & set = elem.second;

        std::string qryName = GetQueryName(qry);
        ret[qryName] = Json::Value(Json::arrayValue);

        for( const auto & slot : set ) {
            Json::Value nVal = am.GetAttributeName(slot);
            ret[qryName].append(nVal);
        }
    }

    return ret;
}

bool LT_GT::GetConfig(WayPointConfigureData& where){

    // get the ID
    WayPointID gfIDOne = GetId ();

    // first, get the function we will send to it
    //    WorkFunc tempFunc = NULL;
    GTPreProcessWorkFunc GTPreProcessWF(NULL);
    GTProcessChunkWorkFunc GTProcessChunkWF (NULL);
    WorkFuncContainer myGLAWorkFuncs;
    myGLAWorkFuncs.Insert (GTPreProcessWF);
    myGLAWorkFuncs.Insert (GTProcessChunkWF);

    // this is the set of query exits that end at it, and flow through it
    QueryExitContainer myGLAEndingQueryExits;
    QueryExitContainer myGLAFlowThroughQueryExits;
    GetQueryExits (myGLAFlowThroughQueryExits, myGLAEndingQueryExits);
    PDEBUG("Printing query exits for GT WP ID = %s", gfIDOne.getName().c_str());
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


    // here is the waypoint configuration data
    GTConfigureData gfConfigure (gfIDOne, myGLAWorkFuncs,  myGLAEndingQueryExits, myGLAFlowThroughQueryExits,
            myReqStates);

    // and add it
    where.swap (gfConfigure);

    return true;
}

void LT_GT::DeleteQuery(QueryID query)
{
    DeleteQueryCommon(query); // common data
    synthesized.erase(query);
    QueryID qID;
    SlotContainer slotCont;
    infoMap.erase(query);
}

void LT_GT::ClearAllDataStructure() {
    ClearAll(); // common data
    gfAttribs.Clear();
    synthesized.clear();
    infoMap.clear();
}

bool LT_GT::AddGT(QueryID query,
        SlotContainer& resultAtts, /*list of attributes produced as the result */
        SlotSet& atts, /* set of attributes mentioned in the expression */
        StateSourceVec & sVec,
        Json::Value& expr /* expression encoding all info to be transmited to code generator */
        )
{
    SlotSet attsSet;
    FOREACH_TWL(iter, resultAtts){
        attsSet.insert(iter);
    }END_FOREACH;
    QueryID qCopy = query;
    gfAttribs.Insert(qCopy, resultAtts);

    stateSources[query] = sVec;
    infoMap[query] = expr;
    CheckQueryAndUpdate(query, atts, newQueryToSlotSetMap);
    queriesCovered.Union(query);
    //synthesized
    CheckQueryAndUpdate(query, attsSet, synthesized);

    return true;
}

// atts coming from top is dropped as it will not be propagated down
// that also means, all attributes coming from top are synthesized ones
// 1. used = used + new queries attributes filled after analysis
// 2. result = used + attributes coming from above - synthesized
bool LT_GT::PropagateDown(QueryID query, const SlotSet& atts, SlotSet& result, QueryExit qe)
{
    CheckQueryAndUpdate(newQueryToSlotSetMap, used);
    newQueryToSlotSetMap.clear();
    result.clear();

    SlotSet uni;
    SlotSet pass;

    // uni = All attributes required above + attribues I need
    set_union(
        atts.begin(), atts.end(),
        used[query].begin(), used[query].end(),
        inserter(uni, uni.begin()));

    // result = All attributes needed - synthesized
    // Attribues needed from below
    set_difference(
        uni.begin(), uni.end(),
        synthesized[query].begin(), synthesized[query].end(),
        inserter(result, result.begin()));

    // pass = Attribues needed from above - synthesized
    // Attributes passed through to above
    set_difference(
        atts.begin(), atts.end(),
        synthesized[query].begin(), synthesized[query].end(),
        inserter(pass, pass.begin()));

    passThrough[query] = pass;
    inputNeeded[query] = result;

    queryExit.Insert (qe);
    return true;
}

bool LT_GT::PropagateDownTerminating(QueryID query, const SlotSet& atts, SlotSet& result, QueryExit qe) {
    queryExitTerminating.Insert(qe);
    return true;
}

// Implementation bottom -> up as follows for all queries together:
// 1. used = used + new queries attributes added since last analysis
// 2. result = attributes coming from below + synthesized
// 3. Dont need to add new attributes received since last analysis to the result
// 4. Correctness : used is subset of what is coming from below
bool LT_GT::PropagateUp(QueryToSlotSet& result)
{
    CheckQueryAndUpdate(newQueryToSlotSetMap, used);
    newQueryToSlotSetMap.clear();
    result.clear();

    // result = attributes coming from below + synthesized
    CheckQueryAndUpdate(downAttributes, result);
    CheckQueryAndUpdate(synthesized, result);

    // used should be subset of what is coming from below
    if (!IsSubSet(used, downAttributes))
    {
        cout <<  "GTWP : Attribute mismatch : used is not subset of attributes coming from below\n";
        return false;
    }
    return true;
}


Json::Value LT_GT::GetJson(){
    Json::Value out(Json::objectValue); // overall object to be return

    out[J_NAME] = GetWPName();
    out[J_TYPE] = JN_GT_WP;

    out[J_PAYLOAD] = MapToJson(infoMap);
    out[J_PASSTHROUGH] = QueryToSlotSetJson(passThrough);

    // print the Attributes with QueryIDSets in which they are used
    // format: (att_name, QueryIDSet_serialized), ..
    SlotToQuerySet reverse;
    AttributesToQuerySet(inputNeeded, reverse);
    out[J_ATT_MAP] = JsonAttToQuerySets(reverse);

    return out;
}

