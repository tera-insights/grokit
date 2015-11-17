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
#include "LT_Scanner.h"
#include "AttributeManager.h"
#include "WayPointConfigureData.h"
#include "ScannerConfig.h"
#include "Catalog.h"

#include <string>
#include <iostream>
#include <sstream>

using namespace std;

// Debugging functions
namespace {

    string PrintSlotToSlotMap(SlotToSlotMap & map ) {
        ostringstream ss;

        bool first = true;
        FOREACH_EM(from, to, map) {
            if( first )
                first = false;
            else
                ss << ", ";

            ss << from.GetInt() << " -> " << to.GetInt();
        } END_FOREACH;

        return ss.str();
    }

}

bool LT_Scanner::GetConfig(WayPointConfigureData& where){
    // all query exit pairs
    QueryExitContainer queryExits;

    // dropped queries
    QueryExitContainer qExitsDone;
    // Alin: not working properly. Returns current queries                    GetDroppedQueries(qExitsDone);

    // columns to slot?
    SlotToSlotMap columnsToSlotsMap;
    AttributeManager& am=AttributeManager::GetAttributeManager();
    am.GetColumnToSlotMapping(GetId().getName(), columnsToSlotsMap);

    SlotToSlotMap storeColumnsToSlots;

    // For each column in the normal mapping, translate the logical slot to the
    // one desired by storeMap.
    // By default, the mapping will be the same as columnsToSlotMap
    FOREACH_EM(physical, logical, columnsToSlotsMap) {
        SlotID newPhysical = physical;
        SlotID newLogical;
        if( storeMap.IsThere(logical) ) {
            newLogical = storeMap.Find(logical);
        } else {
            newLogical = logical;
        }

        storeColumnsToSlots.Insert(newPhysical, newLogical);
    } END_FOREACH;

    // query to slot map
    QueryExitToSlotsMap queryColumnsMap;
    GetQuerExitToSlotMap(queryColumnsMap);

    WayPointID tableID = GetId ();

    // Clustering attribute
    Schema mySchema;
    Catalog& catalog = Catalog::GetCatalog();
    FATALIF(!catalog.GetSchema(relation, mySchema),
        "Unable to find Schema for relation %s", relation.c_str());

    Attribute clusterAtt;
    bool isClustered = mySchema.GetClusterAttribute(clusterAtt);

    SlotID clusterSlot;
    if( isClustered ) {
        clusterSlot = am.GetAttributeSlot(relation, clusterAtt.GetName());        
    }

    QueryToScannerRangeList filterRanges;
    filterRanges = filters;

    /* crap from common inheritance from waypoint*/
    WorkFuncContainer myTableScanWorkFuncs;

    QueryExitContainer myTableScanEndingQueryExits;
    QueryExitContainer myTableScanFlowThroughQueryExits;
    GetQueryExits(queryExits, myTableScanEndingQueryExits);
    PDEBUG("Printing query exits for SCANNER WP ID = %s", tableID.getName().c_str());
#ifdef DEBUG
        cout << "\nFlow through query exits\n" << flush;
        myTableScanFlowThroughQueryExits.MoveToStart();
        while (myTableScanFlowThroughQueryExits.RightLength()) {
                (myTableScanFlowThroughQueryExits.Current()).Print();
                myTableScanFlowThroughQueryExits.Advance();
        }
        cout << "\nEnding query exits\n" << flush;
        myTableScanEndingQueryExits.MoveToStart();
        while (myTableScanEndingQueryExits.RightLength()) {
                (myTableScanEndingQueryExits.Current()).Print();
                myTableScanEndingQueryExits.Advance();
        }
        cout << "\nAll scanning query exits\n" << flush;
        queryExits.MoveToStart();
        while (queryExits.RightLength()) {
                (queryExits.Current()).Print();
                queryExits.Advance();
        }
        cout << endl;
#endif


    /* end crap */
    TableConfigureData scannerConfig( tableID,
            /* crap arguments */
            myTableScanWorkFuncs,
            myTableScanEndingQueryExits,
            myTableScanFlowThroughQueryExits,
            /* end crap */
            relation, queryExits, qExitsDone,
            queryColumnsMap, columnsToSlotsMap,
            storeColumnsToSlots,
            clusterSlot,
            filterRanges);

    where.swap(scannerConfig);

    return true;
}

// This scanner will only get queries from text loader, for text loader this will not be
// called as they are bottom most nodes
void LT_Scanner::ReceiveAttributes(QueryToSlotSet& atts) {

    // TODO: atts should be a subset of the source slots in storeMap
    // Make sure this is the case

    SlotSet storeAtts;
    FOREACH_EM(relSlot, sourceSlot, storeMap) {
        SlotID sourceCopy = sourceSlot;
        storeAtts.insert(sourceCopy);
    } END_FOREACH;

    // allAttr is initialized at the time of constructor
    if (!IsSubSet(storeAtts, atts))
    {
        cout << "ERROR Scanner WP : Attributes in scanner should be subset of attributes coming from textLoader\n";
        return;
    }

    // Save all attributes coming from text loader without any check if this scanner has that query or not
    for (QueryToSlotSet::const_iterator iter = atts.begin();
            iter != atts.end();
            ++iter) {

        QueryID query = iter->first;
        // Just get all of them from textloader
        //if (DoIHaveQueries(query))  {
        SlotSet atts_s = iter->second;
        CheckQueryAndUpdate(query, atts_s, fromTextLoader);
        //}
    }
}

// Add writer capabilities to this scanner
bool LT_Scanner::AddWriter(QueryID query, SlotToSlotMap & qStoreMap){

    storeMap.copy(qStoreMap);
    //cout << "[" << GetId().getName() << "] AddWriter - storeMap: " << PrintSlotToSlotMap(storeMap) << endl;

    SlotSet storeAtts;
    FOREACH_EM(relSlot, sourceSlot, storeMap) {
        SlotID sourceCopy = sourceSlot;
        storeAtts.insert(sourceCopy);
    } END_FOREACH;

    CheckQueryAndUpdate(query, storeAtts, newQueryToSlotSetMap);
    queriesCovered.Union(query);
    return true;
}

bool LT_Scanner::AddScannerRange(QueryID query, int64_t min, int64_t max) {
    filters[query].push_back(ScannerRange(min, max));
    return true;
}

// This is called just before analysis to add all the queries to the scanner waypoint
bool LT_Scanner::AddScanner(QueryIDSet query)
{
    // In case AddWriter is used to add queries one by one, dont use this method
    // to add all queries, since we don't want to overwrite
    Bitstring64 tmp = query.Clone ();
    while (!tmp.IsEmpty()) {
        Bitstring64 q = tmp.GetFirst ();
        CheckQueryAndUpdate(q, allAttr, newQueryToSlotSetMap);
        queriesCovered.Union(query);
    }

    storeMap.MoveToStart();
    if( storeMap.AtEnd() ) {
        for( SlotID elem : allAttr ) {
            SlotID att = elem;
            SlotID attCopy = elem;
            storeMap.Insert(att, attCopy);
        }
    }

    //cout << "[" << GetId().getName() << "] AddScanner - storeMap: " << PrintSlotToSlotMap(storeMap) << endl;
    return true;
}

void LT_Scanner::DeleteQuery(QueryID query) {
    DeleteQueryCommon(query);
    dropped.erase(query);
}

void LT_Scanner::ClearAllDataStructure() {
    ClearAll();
    allAttr.clear();
    dropped.clear();
}

void LT_Scanner::GetDroppedQueries(QueryExitContainer& qe) {
    QueryExitContainer tmp;
    queryExit.MoveToStart();
    // iterate over queryExit and see it some if it matches with dropped queries
    while (queryExit.RightLength()) {
        QueryToSlotSet::const_iterator it = dropped.find((queryExit.Current()).query);
        if (it != dropped.end()){
            tmp.Insert(queryExit.Current());
        }
        queryExit.Advance();
    }
    tmp.swap(qe);
}

void LT_Scanner::GetQuerExitToSlotMap(QueryExitToSlotsMap& qe){
    AttributeManager& am = AttributeManager::GetAttributeManager();
    QueryExitToSlotsMap tmp;
    queryExit.MoveToStart();
    // iterate over queryExit and see it some if it matches with used queries
    // actually all of it should match
    while (queryExit.RightLength()) {
        QueryToSlotSet::const_iterator it = used.find((queryExit.Current()).query);
        if (it != used.end()){
            SlotContainer slotIds;
            for (SlotSet::const_iterator iter = (it->second).begin(); iter != (it->second).end(); ++iter){
                SlotID s = *iter;
                slotIds.Insert(s);
            }
            QueryExit qeTemp = queryExit.Current();
            tmp.Insert(qeTemp, slotIds);
        }
        queryExit.Advance();
    }
    tmp.swap(qe);
}

// Implementation top -> down as follows per query:
// 1. used = used + new queries attributes added since last analysis
// 2. result = used attributes - attributes coming from above
// 3. result can be considered as dropped attributes which can be dropped
//    from further scanning. result can be stored in this class.
// 5. Save new query exit pair per query
bool LT_Scanner::PropagateDown(QueryID query, const SlotSet& atts, SlotSet& rez, QueryExit qe)
{
    CheckQueryAndUpdate(newQueryToSlotSetMap, used);
    newQueryToSlotSetMap.clear();

    if (!IsSubSet(atts, used[query]))
    {
        cout << "Scanner WP : Attributes coming from above should be subset of used\n";
        return false;
    }
    SlotSet result;
    set_difference((used[query]).begin(), (used[query]).end(), atts.begin(), atts.end(), inserter(result, result.begin()));
    dropped[query] = result;
    used[query]=atts;
    // this is flow through query exit, but should never be the case
    queryExit.Insert(qe);
    return true;
}

bool LT_Scanner::PropagateDownTerminating(QueryID query, const SlotSet& atts/*blank*/, SlotSet& result, QueryExit qe)
{
    CheckQueryAndUpdate(newQueryToSlotSetMap, used);
    newQueryToSlotSetMap.clear();
    // populate everything we got from text loader to send them down
    result = used[query]; //fromTextLoader[query];
    queryExitTerminating.Insert(qe);

    std::cerr << "[WRITER] " << GetWPName() << " query " << GetQueryName(query) << ":\n"
        << "\tNeeded By Self: " << GetAllAttrAsString(used[query]) << "\n"
        << "\tRequested From Below: " << GetAllAttrAsString(result) << "\n";

    return true;
}

// Implementation bottom -> up as follows for all queries together:
// 1. used = used + new queries attributes added since last analysis
// 2. result = used (= attributes going up)
bool LT_Scanner::PropagateUp(QueryToSlotSet& result)
{
    // newQueryToSlotSetMap is populated from allAttr while AddScanner
    CheckQueryAndUpdate(newQueryToSlotSetMap, used);
    newQueryToSlotSetMap.clear();
    result.clear();
    result = used;
    if (!downAttributes.empty() && downAttributes == used) {
        cout << "ScannerWP : Attribute mismatch : used is not equal to attribute coming from below textLoader\n";
        return false;
    }
    downAttributes.clear();
    return true;
}

Json::Value LT_Scanner::GetJson() {
    Json::Value out(Json::objectValue);
    out[J_NAME] = GetWPName();
    out[J_TYPE] = "scanner_wp";

    return out;
}
