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
#ifndef _LT_WAYPOINT_H_
#define _LT_WAYPOINT_H_

#include "Catalog.h"
#include "ID.h"
#include "LemonTranslator.h"
#include "JsonAST.h"

#include <cstddef>
#include <set>
#include <vector>
#include <map>
#include <ostream>


std::string GetAllAttrAsString(const std::set<SlotID>& atts);


/** This is the base class for the hierarchy that keeps track of the
        information for the analysis in the Lemon Translator. */

class LT_Waypoint {
public:

    // types that make the life easier
    typedef std::set<SlotID> SlotSet;
    typedef std::map<QueryID, SlotSet> QueryToSlotSet;
    typedef std::map<SlotID, QueryIDSet> SlotToQuerySet;
    typedef std::map<QueryID, Json::Value> QueryToJson; // map queries to expressions. Expressions are generic info in JSON
    typedef std::vector<SlotID> SlotVec;
    typedef std::map<QueryID, SlotVec> QueryToSlotVec;

    //helper functio to transform a QueryToExpression to a JSON array
    Json::Value MapToJson(QueryToJson& list);
    // helper function to produce a list of attributes
    Json::Value JsonAttributeList(const SlotSet& atts);
    Json::Value JsonAttributeList(const SlotVec& atts);

private:
    WayPointID myId; // reverse lookup


public:
    /** The following datastructures are computed/maintained by analysis */

    // All queries mentioned in this waypoint
    QueryIDSet queriesCovered;

    // attributes coming from below waypoint nodes in bottom up analysis
    QueryToSlotSet downAttributes;

    // set of attributes that are used per query in this waypoint
    QueryToSlotSet used;

    // set of attributes per query which are added new since the last analysis
    QueryToSlotSet newQueryToSlotSetMap;

    // what queries to be skipped any processing
    QueryIDSet bypassQueries;

    QueryExitContainer queryExit;
    QueryExitContainer queryExitTerminating;

    // ctor
    LT_Waypoint(WayPointID me):myId(me)
    {}


    /************   non virtual functions  ******************/

    // get wp name
    std::string GetWPName();

    // This function writes the query and associated attributes to the destination (query -> attribute) mapping.
    // If query found, check for any new attributes we have to update. If we have new attributes, add them in old
    // mapping.
    bool CheckQueryAndUpdate(QueryID query, SlotSet& atts, QueryToSlotSet& destination, bool skipByPassQueries = false);

    // This function is same as above, only difference being, it takes a source mapping to update destination map
    // This would be helpful when we process all queries together
    void CheckQueryAndUpdate(QueryToSlotSet& source, QueryToSlotSet& destination, bool skipByPassQueries = false);

    // Check if attribute set of each query in 'subset' is subset of the corrosponding query's attribute set in
    // super set.
    bool IsSubSet(QueryToSlotSet& subset, QueryToSlotSet& superset);

    // Check if attribute set is subset of the corrosponding query's attribute set in
    // super set.
    bool IsSubSet(const SlotSet& subset, QueryToSlotSet& superset);

    // Check if attribute set is subset of attribute set in super set.
    bool IsSubSet(const SlotSet& subset, const SlotSet& superset);

    std::string GetQueryName(QueryID query);

    // display on screen list of attributes
    void PrintAllAttributes(const SlotSet& atts);

    // used to print a comma separated list of attributes in a list
    void PrintAttributeList(const SlotSet& atts, std::ostream& out);

    // used to print pairs attribute, queries used
    void PrintAllQueryAndAttributes(QueryToSlotSet& querySlotMap);


    WayPointID GetId() { return myId;}

    // Does this waypoint have this query? If not, we don't have to follow that route while graph traversal
    bool DoIHaveQueries(QueryIDSet which){
        return queriesCovered.Overlaps(which);
    }

    // clear all common data structures
    void ClearAll();

    // delete query from all common data structures
    void DeleteQueryCommon(QueryID query);


    // go over set atts and keep track of the queries that each attribute
    // participates in. Put result in slotToQuerySet
    void AttributesToQuerySet(QueryToSlotSet& atts, SlotToQuerySet& slotToQuerySet);

    // Print attribute-to-guey-sets in streams.
    // print in format expected by m4 code
    void PrintAttToQuerySets(SlotToQuerySet& slotToQuerySet, std::ostream& out);
    void PrintAttToQuerySetsJoin(SlotToQuerySet& slotToQuerySet, std::ostream& out);

    // functions to map attributes into query sets. the second version fist computes the query sets from
    // slot sets
    Json::Value JsonAttToQuerySets(SlotToQuerySet& input);
    Json::Value JsonAttToQuerySets(QueryToSlotSet& atts);

    // Transforms a QueryIDSet to a list of query names
    Json::Value JsonQuerySet(const QueryIDSet& queries);

    // return flow through and ending query exits
    void GetQueryExits (QueryExitContainer& qeflowthrough, QueryExitContainer& qeEnding);

    void SlotContainerToSet( SlotContainer& cont, SlotSet & result );


    /************   virtual functions  ******************/

    // Store the attributes coming from below waypoint node, this is written little differently for scanners
    virtual void ReceiveAttributes(QueryToSlotSet& atts);

    virtual void ClearAllDataStructure() {};

    // what is type of this waypoint
    virtual WaypointType GetType() {return InvalidWaypoint;}

    // remove queries from all internal datastructures
    virtual void DeleteQuery(QueryID query) {}

    // receive attributes coming from below from terminating edge (join RHS)
    virtual void ReceiveAttributesTerminating(QueryToSlotSet& atts) {}

    virtual bool AddFilter(QueryID query, SlotSet& atts,
            std::vector<WayPointID>& reqStates, Json::Value& info) {
        return false;
    }

    // add a synthesized attribute. Only supported by LT_Select for now
    virtual bool AddSynthesized(QueryID query, SlotID att, SlotSet& atts,
            Json::Value& info) {
        return false;
    }

    //GLA, one per query basis
    virtual bool AddGLA(QueryID query,
                SlotContainer& resultAtts, /*list of attributes produced as the result */
                SlotSet& atts,
                std::vector<WayPointID>& reqStates,
                Json::Value& info ){
        return false;
    }

    // Mark a GLA/GIST as being returned as state
    virtual bool ReturnAsState(QueryID query) {
        return false;
    }

    // GIST, one per query basis
    virtual bool AddGIST( QueryID query,
            SlotContainer& resultAtts,
            std::vector<WayPointID>& reqStates,
            Json::Value& info) {
        return false;
    }

    //GT, one per query basis
    virtual bool AddGT(QueryID query,
                SlotContainer& resultAtts, /*list of attributes produced as the result */
                SlotSet& atts,
                std::vector<WayPointID> & reqStates,
                Json::Value& info){
        return false;
    }

    virtual bool AddBypass(QueryID query) {
        return false;
    }

    virtual bool AddJoin(QueryID query, SlotSet& RHS_atts /* right hand side attributes */,
            SlotVec& RHS_keys, LemonTranslator::JoinType jType, Json::Value& info){
        return false;
    }

    virtual bool AddPrint(QueryID query, SlotSet& atts, Json::Value& info){
        return false;
    }

    virtual bool AddWriter(QueryID query, SlotToSlotMap & storeMap) {
        return false;
    }

    virtual bool AddScannerRange(QueryID query, int64_t min, int64_t max) {
        return false;
    }

    virtual bool AddCaching(QueryID query) {
        return false;
    }

    virtual bool AddScanner(QueryIDSet) {}

    // Should not be needed except scanners
    virtual void GetDroppedQueries(QueryExitContainer& qe) {}

    virtual void GetQuerExitToSlotMap(QueryExitToSlotsMap& qe) {}

    // interface for the analysis

    // analyze top-down the attribute usage (waypoint specific)
    // it is up to each waypiont to correctly propagate down the analysis
    virtual bool PropagateDown(QueryID query, const SlotSet& atts, SlotSet& rez, QueryExit qe){
        return false;
    }

    virtual bool PropagateDownTerminating(QueryID query, const SlotSet& atts, SlotSet& rez, QueryExit qe){
        return false;
    }

    virtual bool PropagateUp(QueryToSlotSet& result) {
        return false;
    }

    // not needed in lemon translator until we need more parameters. Right now its handled in lemon tr
    //virtual void GetSymbolicWPConfig(SymbolicWaypointConfig& wpConfig);


    // Produce the configuration object for corresponding waypoint
    // return false if no new config needed
    // config is placed in where or false is returned
    virtual bool GetConfig(WayPointConfigureData& where){ return false; }
    virtual bool GetConfigs(WayPointConfigurationList& where) {return false;}

    virtual Json::Value GetJson() { return Json::Value(Json::nullValue); }

    virtual void GetAccumulatedLHSRHS(std::set<SlotID>& LHS, std::set<SlotID>& RHS, QueryIDSet& queries) {return ;}


    virtual ~LT_Waypoint(void){}


};


#endif // _LT_WAYPOINT_H_
