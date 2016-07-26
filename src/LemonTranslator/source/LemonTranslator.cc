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
#include "AttributeManager.h"
#include "LemonTranslator.h"
#include <lemon/bfs.h>
#include <lemon/core.h>
#include <lemon/connectivity.h>
#include "LT_Waypoint.h"
#include "LT_Scanner.h"
#include "LT_Selection.h"
#include "LT_Join.h"
#include "LT_Print.h"
#include "LT_TextLoader.h"
#include "LT_GLA.h"
#include "LT_GT.h"
#include "LT_GIST.h"
#include "LT_GI.h"
#include "LT_Cache.h"
#include "LT_Compact.h"
#include "LT_Cluster.h"
#include "AttributeManager.h"
#include "QueryManager.h"
#include "Errors.h"
#include "Debug.h"
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include "ExternalCommands.h"
#include "ContainerTypes.h"
#include "RemoteAddress.h"
#include "CommunicationFramework.h"
#include "QueryPlanMessages.h"

#include <cstddef>
#include <vector>

using namespace lemon;
using namespace std;

namespace {

    string GetAllAttrAsString(SlotContainer& atts) {
        AttributeManager &am = AttributeManager::GetAttributeManager();

        int len = atts.Length();
        atts.MoveToStart();
        string rez;
        while (!atts.AtEnd()){
            SlotID slot = atts.Current();
            rez += " ";
            rez += am.GetAttributeName(slot);
            atts.Advance();
        }
        return rez;
        //cout << "\nLHS: " << (l.getIDAsString()).c_str();
    }

    string GetAllAttrAsString(SlotID& atts) {
        string rez;
        IDInfo l;
        (const_cast<SlotID&>(atts)).getInfo(l);
        rez += (l.getName()).c_str();
        return rez;
    }

    void ConvertToSTLSet (SlotContainer& atts, set<SlotID>& attr) {
        int len = atts.Length();
        attr.clear();
        atts.MoveToStart();
        while (!atts.AtEnd()){
            SlotID slot = atts.Current();
            attr.insert(slot);
            atts.Advance();
        }
    }

    template<class T>
    void ListToVector(TwoWayList<T>& in, std::vector<T>& out)
    {
        out.clear();
        in.MoveToStart();
        while (!in.AtEnd()) {
            T tmp = in.Current();
            out.push_back(tmp);
            in.Advance();
        }
    }

}

void LemonTranslator::AddHeader( Json::Value val ) {
    headers.append(val);
}

void LemonTranslator::AddTask( Task & t ) {
    tasks.Append(t);
}

void LemonTranslator :: SetJobID( const std::string & j ) {
    jobID = j;
}

void LemonTranslator::PrintDOT(ostream& out){

    // preamble
    out <<  "\n\t/* Graph */\n";
    out <<  "digraph G {\n\tcompound=true;\n\trankstep=1.25;\n";
    out << "\tlabel=\"LEMON Data Path Network\";\n\tnode[shape=ellipse,fontsize=12,fontcolor=black,color=grey];\n";
    out << "\tbgcolor=white;\n\tedge [arrowsize=1,fontsize=10];\n";

    // nodes
    out << "\n\t/* Nodes */\n";
    {
        Bfs<ListDigraph> bfs(graph);
        bfs.init();
        for (ListDigraph::NodeIt n(graph); n != INVALID; ++n) {
            if (n != topNode && n != bottomNode) {
                LT_Waypoint* wp = nodeToWaypointData[n];
                string wName = wp->GetWPName();

                out << "\tsubgraph cluster_" << wName << " {label=\""
                    << wName << "\"; labelloc=\"b\";};\n";
            }
            if (!bfs.reached(n)) {
                bfs.addSource(n);
                bfs.start();
            }
        }
    }
    // edges
    out << "\n\t/* Relationships */\n";
    // terminating edge

    // Traverse full graph step by step (lemon BFS algo) and fill the info
    Bfs<ListDigraph> bfs(graph);
    bfs.init();
    for (ListDigraph::NodeIt n(graph); n != INVALID; ++n) {
        if (n != topNode && n != bottomNode) {
            LT_Waypoint* wp = nodeToWaypointData[n];
            string nID = wp->GetWPName();

            // Now find all the outlinks for the node
            for (ListDigraph::OutArcIt arc(graph, n); arc != INVALID; ++arc) {
                ListDigraph::Node next = graph.target(arc);
                bool isTerminating = terminatingArcMap[arc];
                if (next != topNode && next != bottomNode) {
                    LT_Waypoint* nextWP = nodeToWaypointData[next];
                    string nTopGuyID = nextWP->GetWPName();

                    // Get the top guy ID
                    WayPointID topGuyID = nextWP->GetId();

                    QueryExitContainer queryExitsFlowThrough;
                    QueryExitContainer queryExitsEnding;
                    nextWP->GetQueryExits(queryExitsFlowThrough, queryExitsEnding);

                    if (!isTerminating) {
                        string QEs;
                        queryExitsFlowThrough.MoveToStart ();
                        while (queryExitsFlowThrough.RightLength ()) {
                            QueryExit qe = queryExitsFlowThrough.Current ();
                            QEs += qe.GetStr();
                            queryExitsFlowThrough.Advance ();
                        }
                        if (QEs == "")
                            QEs = "error";
                        out <<     "\tedge [arrowsize=1,color=blue,label=\"" << QEs << "\"" << "]\t "
                            << nID << "->" << nTopGuyID << "\n";
                    } else {
                        string QEs;
                        queryExitsEnding.MoveToStart ();
                        while (queryExitsEnding.RightLength ()) {
                            QueryExit qe = queryExitsEnding.Current ();
                            QEs += qe.GetStr();
                            queryExitsEnding.Advance ();
                        }
                        if (QEs == "")
                            QEs = "error";
                        out <<     "\tedge [arrowsize=1,color=red,label=\"" << QEs << "\"" << "]\t "
                            << nID << "->" << nTopGuyID << "\n";
                    }
                }
            }

        }
        if (!bfs.reached(n)) {
            bfs.addSource(n);
            bfs.start();
        }
    }

    // finish
    out << "\n}\n";

}

// We create top and bottom nodes for the graph, this will serve as starting and
// termination points
// we also create the clearner waypoint
LemonTranslator::LemonTranslator( bool batchMode ) :
    terminatingArcMap(graph),
    cleanerID("Cleaner"),
    batchMode(batchMode)
{
    topNode = graph.addNode();
    bottomNode = graph.addNode();
    isNewRun = false;
    headers = Json::Value(Json::arrayValue);
}

// Common processing function
// This will be called from each add waypoint function to add graph node
bool LemonTranslator::AddGraphNode(WayPointID WPID, WaypointType type, LT_Waypoint*& WP) {

    ListDigraph::Node u = graph.addNode();
    IDToNode[WPID] = u;

    if (type == PrintWaypoint || type == ClusterWaypoint){
        graph.addArc(u, topNode);
    } else if (type == ScannerWaypoint){
        graph.addArc(bottomNode, u);
    } else if (type == TextLoaderWaypoint){
        graph.addArc(bottomNode, u);
    }

    // fill our node to waypoint map
    nodeToWaypointData[u] = WP;
    return true;
}

bool LemonTranslator::AddEdgeFromBottom( WayPointID WPID) {
    PDEBUG("LemonTranslator::AddEdgeFromBottom(WPID = %s)", WPID.getName().c_str());
    FATALIF(!WPID.IsValid(), "Invalid WaypointID received in AddEdgeFromBottom");
    map<WayPointID, ListDigraph::Node>::const_iterator it = IDToNode.find(WPID);
    if( it == IDToNode.end() ) {
        cout << "No node found in the graph for waypoint " << WPID.getName() << endl;
        return false;
    }

    ListDigraph::Arc arc = graph.addArc(bottomNode, it->second);
    terminatingArcMap.set(arc, false);

    return true;
}

bool LemonTranslator::AddEdgeToTop( WayPointID WPID) {
    PDEBUG("LemonTranslator::AddEdgeToTop(WPID = %s)", WPID.getName().c_str());
    FATALIF(!WPID.IsValid(), "Invalid WaypointID received in AddEdgeToTop");
    map<WayPointID, ListDigraph::Node>::const_iterator it = IDToNode.find(WPID);
    if( it == IDToNode.end() ) {
        cout << "No node found in the graph for waypoint " << WPID.getName() << endl;
        return false;
    }

    ListDigraph::Arc arc = graph.addArc(it->second, topNode);
    terminatingArcMap.set(arc, false);

    return true;
}

// Common processing function
// This will be called from each add conditions function, like AddFilter
bool LemonTranslator::GetWaypointAttr(WayPointID WPID, SlotContainer& atts,
        set<SlotID>& attr, LT_Waypoint*& WP) {

    map<WayPointID, ListDigraph::Node>::const_iterator it = IDToNode.find(WPID);
    if (it == IDToNode.end()) {
        cout << "No graph node found for specified waypoint ID";
        return false;
    }
    WP = nodeToWaypointData[it->second];
    if (WP == NULL) {
        cout << "No waypoint details found for given node";
        return false;
    }
    attr.clear();
    atts.MoveToStart();
    while (!atts.AtEnd()){
        SlotID slot = atts.Current();
        attr.insert(slot);
        atts.Advance();
    }
    return true;
}

void LemonTranslator::ClearAllDataStructure()
{
    Bfs<ListDigraph> bfs(graph);
    bfs.init();
    for (ListDigraph::NodeIt n(graph); n != INVALID; ++n) {
        if (n != topNode && n != bottomNode) {
                LT_Waypoint* wp = nodeToWaypointData[n];
                wp->ClearAllDataStructure();
            }
        if (!bfs.reached(n)) {
            bfs.addSource(n);
            bfs.start();
        }
    }
}

string LemonTranslator::GetErrorMessage()
{
    //return error;
}

bool LemonTranslator::DeleteQuery(QueryID qID)
{

    deleteQueries.Union(qID); // remember the deleted queries

    // Traverse full graph step by step (lemon BFS algo) and delete query and related info
    // in each node if found.
    Bfs<ListDigraph> bfs(graph);
    bfs.init();
    for (ListDigraph::NodeIt n(graph); n != INVALID; ++n) {
        if (n != topNode && n != bottomNode) {
                LT_Waypoint* wp = nodeToWaypointData[n];
                wp->DeleteQuery(qID);
            }
        if (!bfs.reached(n)) {
            bfs.addSource(n);
            bfs.start();
        }
    }
}

bool LemonTranslator::AddScannerWP(WayPointID scannerID,
    string relName,
    SlotContainer& atts)
{
    PDEBUG("LemonTranslator::AddScannerWP(WayPointID scannerID = %s, SlotContainer& atts = %s)",
            scannerID.getName().c_str(), (GetAllAttrAsString(atts)).c_str());
    FATALIF(!scannerID.IsValid(), "Invalid WaypointID received in AddScannerWP");
    set<SlotID> attr;
    ConvertToSTLSet (atts, attr);
    LT_Waypoint* WP = new LT_Scanner(scannerID, relName, attr);
    return AddGraphNode(scannerID, ScannerWaypoint, WP);
}


bool LemonTranslator::AddTextLoaderWP(WayPointID loaderID, SlotContainer& atts,
				      Json::Value& info)
{
    FATALIF(!loaderID.IsValid(), "Invalid WaypointID received in AddTextLoaderWP");
    set<SlotID> attr;
    ConvertToSTLSet (atts, attr);
    LT_Waypoint* WP = new LT_TextLoader(loaderID, attr, atts, info);
    return AddGraphNode(loaderID, TextLoaderWaypoint, WP);
}

static std::string StringContainerToString( StringContainer& cont ) {
    std::stringstream result;
    result << "[";

    bool first = true;
    for( StringContainer::iterator it = cont.begin(); it != cont.end(); ++it ) {
        if( first )
            first = false;
        else
            result << ", ";

        result << *it;
    }

    result << "]";

    return result.str();
}

bool LemonTranslator::AddGIWP(WayPointID giID, SlotContainer& attributes, Json::Value& info ) {
    PDEBUG("LemonTranslator::AddGIWP(WayPointID giWP = %s, SlotContainer& attributes = %s)",
            giID.getName().c_str(), (GetAllAttrAsString(attributes)).c_str());
    FATALIF(!giID.IsValid(), "Invalid WayPointID received in GIWP");
    set<SlotID> attr;
    ConvertToSTLSet(attributes, attr);
    LT_Waypoint * WP = new LT_GI(giID, attr, info );
    return AddGraphNode( giID, GIWayPoint, WP );
}

bool LemonTranslator::AddSelectionWP(WayPointID selWPID)
{
    PDEBUG("LemonTranslator::AddSelectionWP(WayPointID selWPID = %s)", selWPID.getName().c_str());
    FATALIF(!selWPID.IsValid(), "Invalid WaypointID received in AddSelectionWP");
    LT_Waypoint* WP = new LT_Selection(selWPID);
    return AddGraphNode(selWPID, SelectionWaypoint, WP);
}

bool LemonTranslator::AddJoinWP(WayPointID joinWPID, SlotContainer& LHS_att, Json::Value& info)
{
    PDEBUG("LemonTranslator::AddJoinWP(WayPointID joinWPID = %s, SlotContainer& LHS_att = %s)", joinWPID.getName().c_str(), (GetAllAttrAsString(LHS_att)).c_str());
    FATALIF(!joinWPID.IsValid(), "Invalid WaypointID received in AddJoinWP");
    set<SlotID> attr;
    std::vector<SlotID> attrVec;
    ConvertToSTLSet (LHS_att, attr);
    ListToVector(LHS_att, attrVec);
    LT_Waypoint* WP = new LT_Join(joinWPID, attr, attrVec, cleanerID, info);
    return AddGraphNode(joinWPID, JoinWaypoint, WP);
}

bool LemonTranslator::AddGLAWP(WayPointID glaWPID)
{
    PDEBUG("LemonTranslator::AddGLAWP(WayPointID glaWPID = %s)", glaWPID.getName().c_str());
    FATALIF(!glaWPID.IsValid(), "Invalid WaypointID received in AddGLAWP");
    LT_Waypoint* WP = new LT_GLA(glaWPID);
    return AddGraphNode(glaWPID, GLAWaypoint, WP);
}

bool LemonTranslator::ReturnAsState(WayPointID wpID, QueryID qID)
{
    PDEBUG("LemonTranslator::ReturnAsState(WayPointID wp = %s)", wpID.getName().c_str());
    FATALIF(!wpID.IsValid(), "Invalid WaypointID received in ReturnAsState");
    LT_Waypoint* WP = NULL;
    SlotContainer atts;
    set<SlotID> attr;
    if (GetWaypointAttr(wpID, atts, attr, WP) == false) return false;
    return WP->ReturnAsState(qID);
}

bool LemonTranslator::AddGTWP(WayPointID gfID) {
    PDEBUG("LemonTranslator::AddGTWP(WayPointID gfId = %s)", gfID.getName().c_str());
    FATALIF(!gfID.IsValid(), "Invalid WayPointID received in AddGTWP");
    LT_Waypoint* WP = new LT_GT(gfID);
    return AddGraphNode(gfID, GTWaypoint, WP);
}

bool LemonTranslator::AddGISTWP(WayPointID gistID) {
    PDEBUG("LemonTranslator::AddGISTWP(WayPointID gistID = %s)", gistID.getName().c_str());
    FATALIF(!gistID.IsValid(), "Invalid WayPointID received in AddGISTWP");
    LT_Waypoint* WP = new LT_GIST(gistID);
    return AddGraphNode(gistID, GISTWayPoint, WP);
}

bool LemonTranslator::AddPrintWP(WayPointID printWPID)
{
    PDEBUG("LemonTranslator::AddPrintWP(WayPointID printWPID = %s)", printWPID.getName().c_str());
    FATALIF(!printWPID.IsValid(), "Invalid WaypointID received in AddPrintWP");
    LT_Waypoint* WP = new LT_Print(printWPID);
    return AddGraphNode(printWPID, PrintWaypoint, WP);
}

bool LemonTranslator :: AddCacheWP(WayPointID wpID) {
    PDEBUG("LemonTranslator::AddCacheWP(WayPointID wpID = %s)", wpID.getName().c_str());
    FATALIF(!wpID.IsValid(), "Invalid WaypointID received in AddPrintWP");
    LT_Waypoint* WP = new LT_Cache(wpID);
    return AddGraphNode(wpID, CacheWaypoint, WP);
}

bool LemonTranslator :: AddCompactWP(WayPointID wpID) {
    PDEBUG("LemonTranslator::AddCompactWP(WayPointID wpID = %s)", wpID.getName().c_str());
    FATALIF(!wpID.IsValid(), "Invalid WaypointID received in AddPrintWP");
    LT_Waypoint* WP = new LT_Compact(wpID);
    return AddGraphNode(wpID, CompactWaypoint, WP);
}

bool LemonTranslator::AddClusterWP(WayPointID wpID,
    std::string relation, SlotID cAtt, QueryID query)
{
    PDEBUG("LemonTranslator::AddClusterWP(WayPointID wpID = %s)", wpID.getName().c_str());
    FATALIF(!wpID.IsValid(), "Invalid WayPointID received in AddClusterWP");
    LT_Waypoint* WP = new LT_Cluster(wpID, relation, cAtt, query);
    auto ret = AddGraphNode(wpID, ClusterWaypoint, WP);

    SlotContainer atts;
    std::set<SlotID> attr;

    SlotID tmp = cAtt;
    atts.Append(tmp);
    if (GetWaypointAttr(wpID, atts, attr, WP) == false) return false;
    queryToRootMap[query] = IDToNode[wpID];

    return ret;
}

bool LemonTranslator::AddEdge(WayPointID start, WayPointID end)
{
    PDEBUG("LemonTranslator::AddEdge(WayPointID start = %s, WayPointID end = %s)", start.getName().c_str(), end.getName().c_str());
    FATALIF(!start.IsValid(), "Invalid WaypointID received in AddEdge");
    FATALIF(!end.IsValid(), "Invalid WaypointID received in AddEdge");
    map<WayPointID, ListDigraph::Node>::const_iterator itStart = IDToNode.find(start);
    if (itStart == IDToNode.end()) {
        cout << "No start node found in graph for specified waypoint ID";
        return false;
    }
    map<WayPointID, ListDigraph::Node>::const_iterator itEnd = IDToNode.find(end);
    if (itEnd == IDToNode.end()) {
        cout << "No end node found in graph for specified waypoint ID";
        return false;
    }
    ListDigraph::Arc arc = graph.addArc(itStart->second, itEnd->second);
    terminatingArcMap.set(arc, false);

    return true;
}

bool LemonTranslator::AddTerminatingEdge(WayPointID start, WayPointID end)
{
    PDEBUG("LemonTranslator::AddTerminatingEdge(WayPointID start = %s, WayPointID end = %s)", start.getName().c_str(), end.getName().c_str());
    FATALIF(!start.IsValid(), "Invalid WaypointID received in AddTerminatingEdge");
    FATALIF(!end.IsValid(), "Invalid WaypointID received in AddTerminatingEdge");
    map<WayPointID, ListDigraph::Node>::const_iterator itStart = IDToNode.find(start);
    if (itStart == IDToNode.end()) {
        cout << "No start node found in graph for specified waypoint ID";
        return false;
    }
    map<WayPointID, ListDigraph::Node>::const_iterator itEnd = IDToNode.find(end);
    if (itEnd == IDToNode.end()) {
        cout << "No end node found in graph for specified waypoint ID";
        return false;
    }
    ListDigraph::Arc arc = graph.addArc(itStart->second, itEnd->second);

    terminatingArcMap.set(arc, true);

    return true;
}

bool LemonTranslator::AddCaching(WayPointID wpID, QueryID queryID) {
    FATALIF(!wpID.IsValid(), "Invalid WaypointID received in AddCaching");
    LT_Waypoint* WP = NULL;
    SlotContainer atts;
    set<SlotID> attr;
    if (GetWaypointAttr(wpID, atts, attr, WP) == false) return false;
    return WP->AddCaching(queryID);
}

bool LemonTranslator::AddCompact(WayPointID wpID, QueryID queryID) {
    FATALIF(!wpID.IsValid(), "Invalid WaypointID received in AddCompact");
    LT_Waypoint* WP = NULL;
    SlotContainer atts;
    set<SlotID> attr;
    if (GetWaypointAttr(wpID, atts, attr, WP) == false) return false;
    return WP->AddCompact(queryID);
}

// Selection, Join
bool LemonTranslator::AddFilter(WayPointID wpID, QueryID queryID, SlotContainer& atts,
    vector<WayPointID> reqStates, Json::Value& info)
{
    PDEBUG("LemonTranslator::AddFilter(WayPointID wpID = %s, QueryID queryID = %s, SlotContainer& atts = %s)",
         wpID.getName().c_str(), queryID.ToString().c_str(), (GetAllAttrAsString(atts)).c_str());
    FATALIF(!wpID.IsValid(), "Invalid WaypointID received in AddFilter");
    LT_Waypoint* WP = NULL;
    set<SlotID> attr;
    if (GetWaypointAttr(wpID, atts, attr, WP) == false) return false;
    return WP->AddFilter(queryID, attr, reqStates, info);
}

bool LemonTranslator::AddSynthesized(WayPointID wpID, QueryID queryID, SlotID att,
        SlotContainer& atts, Json::Value& info)
{
    //PDEBUG("LemonTranslator::AddSynthesized(WayPointID wpID = %s, QueryID queryID = %s, SlotID attr = %s)", wpID.getName().c_str(), queryID.ToString().c_str(), (GetAllAttrAsString(attr)).c_str());
    FATALIF(!wpID.IsValid(), "Invalid WaypointID received in AddSynthesized");
    LT_Waypoint* WP = NULL;
    map<WayPointID, ListDigraph::Node>::const_iterator it = IDToNode.find(wpID);
    if (it == IDToNode.end()) {
        cout << "No graph node found for specified waypoint ID";
        return false;
    }
    WP = nodeToWaypointData[it->second];
    if (WP == NULL) {
        cout << "No waypoint details found for gven node";
        return false;
    }
    set<SlotID> attr;
    if (GetWaypointAttr(wpID, atts, attr, WP) == false) return false;
    return WP->AddSynthesized(queryID, att, attr, info);
}

//GLA, one per query basis
bool LemonTranslator::AddGLA(WayPointID wpID, QueryID query,
        SlotContainer& resultAtts, /*list of attributes produced as the result */
        SlotContainer& atts,
        vector<WayPointID>& reqStates,
        Json::Value& info)
{
    PDEBUG("LemonTranslator::AddGLA(WayPointID wpID = %s, QueryID query = %s, SlotContainer resultAtts = %s, SlotContainer& atts = %s)", wpID.getName().c_str(), query.ToString().c_str(), (GetAllAttrAsString(resultAtts)).c_str(), (GetAllAttrAsString(atts)).c_str());
    FATALIF(!wpID.IsValid(), "Invalid WaypointID received in AddAggregate");
    LT_Waypoint* WP = NULL;
    set<SlotID> attr;
    if (GetWaypointAttr(wpID, atts, attr, WP) == false) return false;
    bool ret = WP->AddGLA(query, resultAtts, attr, reqStates, info);

    return ret;
}

//GT, one per query basis
bool LemonTranslator::AddGT(WayPointID wpID, QueryID query,
        SlotContainer& resultAtts, /*list of attributes produced as the result */
        SlotContainer& atts,
        vector<WayPointID> reqStates,
        Json::Value& info)
{
    PDEBUG("LemonTranslator::AddGT(WayPointID wpID = %s, QueryID query = %s, SlotContainer resultAtts = %s, SlotContainer& atts = %s)", wpID.getName().c_str(), query.ToString().c_str(), (GetAllAttrAsString(resultAtts)).c_str(), (GetAllAttrAsString(atts)).c_str());
    FATALIF(!wpID.IsValid(), "Invalid WaypointID received in AddGT");
    LT_Waypoint* WP = NULL;
    set<SlotID> attr;
    if (GetWaypointAttr(wpID, atts, attr, WP) == false) return false;
    return WP->AddGT(query, resultAtts, attr, reqStates, info);
}

//GIST, one per query basis
bool LemonTranslator::AddGIST(WayPointID wpID, QueryID query,
        SlotContainer& resultAtts, /*list of attributes produced as the result */
        vector<WayPointID> reqStates,
        Json::Value& info)
{
    PDEBUG("LemonTranslator::AddGIST(WayPointID wpID = %s"
            ", QueryID query = %s"
            ", SlotContainer resultAtts = %s"
            ")",
            wpID.getName().c_str(),
            query.ToString().c_str(),
            GetAllAttrAsString(resultAtts).c_str());

    FATALIF(!wpID.IsValid(), "Invalid WaypointID received in AddGT");
    LT_Waypoint* WP = NULL;
    SlotContainer atts; // dummy
    set<SlotID> attr;
    bool gotWP = GetWaypointAttr(wpID, atts, attr, WP);
    if (!gotWP)
        return false;
    bool success = WP->AddGIST(query, resultAtts, reqStates, info);
    return success;
}

// Selection, Join. Queries added one by one
bool LemonTranslator::AddBypass(WayPointID wpID, QueryID query)
{
    PDEBUG("LemonTranslator::AddBypass(WayPointID wpID = %s, QueryID query %s)", wpID.getName().c_str(), query.ToString().c_str());
    FATALIF(!wpID.IsValid(), "Invalid WaypointID received in AddBypass");
    LT_Waypoint* WP = NULL;
    SlotContainer atts; // dummy
    set<SlotID> attr; // dummy
    if (GetWaypointAttr(wpID, atts, attr, WP) == false) return false;
    return WP->AddBypass(query);
}

// Join
bool LemonTranslator::AddJoin(WayPointID wpID, QueryID query,
        SlotContainer& RHS_atts /* right hand side attributes */,
        LemonTranslator::JoinType jType,
        Json::Value& info)
{
    PDEBUG("LemonTranslator::AddJoin(WayPointID wpID = %s, QueryID query = %s, SlotContainer& RHS_atts = %s)", wpID.getName().c_str(), query.ToString().c_str(), (GetAllAttrAsString(RHS_atts)).c_str());
    FATALIF(!wpID.IsValid(), "Invalid WaypointID received in AddJoin");
    LT_Waypoint* WP = NULL;
    set<SlotID> attr;
    std::vector<SlotID> attrVec;
    if (GetWaypointAttr(wpID, RHS_atts, attr, WP) == false) return false;
    ListToVector(RHS_atts, attrVec);
    return WP->AddJoin(query, attr, attrVec, jType, info);
}


// Print
bool LemonTranslator::AddPrint(WayPointID wpID, QueryID query,
        SlotContainer& atts, Json::Value& info)
{
    PDEBUG("LemonTranslator::AddPrint(WayPointID wpID = %s, QueryID query = %s, SlotContainer& atts = %s)", wpID.getName().c_str(), query.ToString().c_str(), (GetAllAttrAsString(atts)).c_str());
    FATALIF(!wpID.IsValid(), "Invalid WaypointID received in AddPrint");
    LT_Waypoint* WP = NULL;
    set<SlotID> attr;
    if (GetWaypointAttr(wpID, atts, attr, WP) == false) return false;
    queryToRootMap[query] = IDToNode[wpID];
    return WP->AddPrint(query, attr, info);
}

// Add writing capabilities to a scanner
// query specifies which query the scanner acts as a writer
bool LemonTranslator::AddWriter(WayPointID wpID, QueryID query, SlotToSlotMap& storeMap){
    PDEBUG("LemonTranslator::AddWriter(WayPointID wpID, QueryID query)");
    FATALIF(!wpID.IsValid(), "Invalid WaypointID received in AddWriter");
    LT_Waypoint* WP = NULL;
    set<SlotID> attr;
    SlotContainer atts;
    if (GetWaypointAttr(wpID, atts, attr, WP) == false) return false;
    queryToRootMap[query] = IDToNode[wpID];
    return WP->AddWriter(query, storeMap);
}

bool LemonTranslator::AddScanner(WayPointID wpID, QueryID query){
   PDEBUG("LemonTranslator::AddScanner(WayPointID wpID, QueryID query)");
    FATALIF(!wpID.IsValid(), "Invalid WaypointID received in AddScanner");
    LT_Waypoint* WP = NULL;
    set<SlotID> attr;
    SlotContainer atts;
    if (GetWaypointAttr(wpID, atts, attr, WP) == false) return false;
    queryToRootMap[query] = IDToNode[wpID];
    return WP->AddScanner(query);
}

bool LemonTranslator::AddScannerRange(WayPointID wpID, QueryID query,
    int64_t min, int64_t max)
{
    PDEBUG("LemonTranslator::AddScannerRange(wpID, query)");
    FATALIF(!wpID.IsValid(), "Invalid WaypointID received in AddScannerRange");
    LT_Waypoint* WP = NULL;
    set<SlotID> attr;
    SlotContainer atts;
    if(GetWaypointAttr(wpID, atts, attr, WP) == false) return false;
    queryToRootMap[query] = IDToNode[wpID];
    return WP->AddScannerRange(query, min, max);
}

bool LemonTranslator::Run(QueryIDSet queries)
{
    PDEBUG("LemonTranslator::Run(QueryIDSet queries = %s)", queries.ToString().c_str());
    ofstream file;
        file.open("DOT.dot");
           PrintDOT(file);
    file.close();


    // Before running analysis bottom up in topological order, check if it is DAG
    // because topological traversal is possible only on DAG
    if (!dag(graph)) {
        cout << "This graph is not Directed acyclic graph";
        return false;
    }
    //ClearAllDataStructure(); otherwise all scanner atts will go away
    // Bottom up traversal required before top down
    AnalyzeAttUsageBottomUp(queries);

    //for each query in queries, do top down analysis
  QueryIDSet tmp = queries.Clone ();
    while (!tmp.IsEmpty())
    {
      QueryID q = tmp.GetFirst ();
      FATALIF(queryToRootMap.find(q) == queryToRootMap.end(), "Query has no top");

        WayPointID wp = nodeToWaypointData[queryToRootMap[q]]->GetId();

    FATALIF(!wp.IsValid(), "No valid top node, what is happening");

        QueryExit exitWP(q, wp);
        set<SlotID> dummy;
        // Call analysis
        AnalyzeAttUsageTopDown(q, queryToRootMap[q], exitWP, dummy);
    }
    isNewRun = true;

    // add the queries to the list of new queries
    newQueries.Union(queries);

    return true;
}

bool LemonTranslator::GetConfig(string dir,
        QueryExitContainer& newQueries,
        DataPathGraph& graph,
        WayPointConfigurationList& wpList,
        TaskList & taskList
        ){
    PDEBUG("LemonTranslator::GetConfig(string dir,DataPathGraph& graph,WayPointConfigurationList& wpList)");

    if (isNewRun) {
        // Generate overall program JSON
        Json::Value programJson = GetJson();

        ofstream jout("query.json");
        jout << programJson;

        jout.close();

        // Do PHP codepath
        GenerateCodeJSON(dir);

        // Send the generation info to the frontend
        if( !batchMode ) {
            ifstream genInfoIn("./Generated/GenerationInfo.json");
            Json::Value genInfo;
            genInfoIn >> genInfo;

            HostAddress frontend;
            GetFrontendAddress(frontend);
            MailboxAddress loggerAddr(frontend, "logger");
            EventProcessor logger;
            FindRemoteEventProcessor(loggerAddr, logger);
            QueryPlanMessage::Factory( logger, genInfo );
        }

        CompileCode(dir);

        PopulateGraph(graph);
        PopulateWayPointConfigurationData(wpList);
        PlotGraph(dir);

        Json::Value configJson;
        ToJson(wpList, configJson);
        ofstream wpout("wpConfig.json");
        wpout << configJson;
        wpout.close();

        taskList.swap(tasks);

        // compute the list of

        isNewRun = false;
        return true;
    } else {
        return false;
    }
}

void LemonTranslator::PlotGraph(string dir){
}

void LemonTranslator::PopulateWayPointConfigurationData(WayPointConfigurationList& myConfigs){

    Bfs<ListDigraph> bfs(graph);
    bfs.init();
    for (ListDigraph::NodeIt n(graph); n != INVALID; ++n) {
        if (n != topNode && n != bottomNode) {
                LT_Waypoint* wp = nodeToWaypointData[n];

                //WayPointConfigureData wpConfig;
                //if (wp->GetConfig(wpConfig))
                //    myConfigs.Insert(wpConfig);
                // Below code should be made active and above if condition must be commented
                // JOINMERGE
                WayPointConfigureData wpConfig;
                WayPointConfigurationList confList;
                if (wp->GetType() != JoinWaypoint && wp->GetConfig(wpConfig))
                     myConfigs.Insert(wpConfig);
                else if (wp->GetConfigs(confList)) {
                    confList.MoveToStart();
                    while (confList.RightLength()) {
                        myConfigs.Insert(confList.Current());
                        confList.Advance();
                    }
                }
            }
        if (!bfs.reached(n)) {
            bfs.addSource(n);
            bfs.start();
        }
    }

    // Now deal with the cleaner
    WorkFunc tempFunc = NULL;
    CleanerWorkFunc myCleanerWorkFunc (tempFunc);
    WorkFuncContainer myCleanerWorkFuncs;
    myCleanerWorkFuncs.Insert (myCleanerWorkFunc);

    // this is the set of query exits that end at it, and flow through it (none!)
    QueryExitContainer myCleanerEndingQueryExits;
    QueryExitContainer myCleanerFlowThroughQueryExits;

    // here is the waypoint configuration data
    HashTableCleanerConfigureData cleanerConfigure (cleanerID, myCleanerWorkFuncs,
        myCleanerEndingQueryExits, myCleanerFlowThroughQueryExits);

    myConfigs.Insert (cleanerConfigure);


}

void LemonTranslator::PopulateWaypoints(SymbolicWPConfigContainer& waypoints){
    SymbolicWPConfigContainer tmp;
    Bfs<ListDigraph> bfs(graph);
    bfs.init();
    for (ListDigraph::NodeIt n(graph); n != INVALID; ++n) {
        if (n != topNode && n != bottomNode) {
                LT_Waypoint* wp = nodeToWaypointData[n];
                SymbolicWaypointConfig symbolicWP(wp->GetType(), wp->GetId());
                //wp->GetSymbolicWPConfig(symbolicWP); TBD for future when more details needed
                tmp.Insert(symbolicWP);
        }
        if (!bfs.reached(n)) {
            bfs.addSource(n);
            bfs.start();
        }
    }
    tmp.swap(waypoints);
}

void LemonTranslator::PopulateGraph(DataPathGraph& rez){
    DataPathGraph g;

    {
    // First iterate and add all nodes, then add edges later in other loop
    Bfs<ListDigraph> bfs(graph);
    bfs.init();
    for (ListDigraph::NodeIt n(graph); n != INVALID; ++n) {
        //if (!bfs.reached(n)) {
            if (n != topNode && n != bottomNode) {
                LT_Waypoint* wp = nodeToWaypointData[n];

                // The waypoint ID
                WayPointID ID = wp->GetId();

                g.AddNode(ID);

                // JOINMERGE
                if (wp->GetType() == JoinWaypoint) {
                    // add join merge waypoint node
                    //string name = ID.getName();
                    //name += "_merge";
                    //WayPointID joinmergeID(name.c_str());
                    //g.AddNode(joinmergeID);
                    // add writer node
                    string namew = ID.getName();
                    namew += "_writer";
                    WayPointID joinwriterID(namew.c_str());
                    g.AddNode(joinwriterID);
                }

    //        }
            if (!bfs.reached(n)) {
                bfs.addSource(n);
                bfs.start();
            }
        }
    }
    g.AddNode(cleanerID);
    }

    // Traverse full graph step by step (lemon BFS algo) and fill the info for edges
    Bfs<ListDigraph> bfs(graph);
    bfs.init();
    for (ListDigraph::NodeIt n(graph); n != INVALID; ++n) {
            if (n != topNode && n != bottomNode) {
                LT_Waypoint* wp = nodeToWaypointData[n];

                // The waypoint ID
                WayPointID ID = wp->GetId();

                //g.AddNode(ID);

                // Now find all the outlinks for the node
                for (ListDigraph::OutArcIt arc(graph, n); arc != INVALID; ++arc) {
                    ListDigraph::Node next = graph.target(arc);
                    bool isTerminating = terminatingArcMap[arc];
                    if (next != topNode && next != bottomNode) {
                        LT_Waypoint* nextWP = nodeToWaypointData[next];

                        // get the query exits of top guy
                        QueryExitContainer queryExitsFlowThrough;
                        QueryExitContainer queryExitsEnding;
                        nextWP->GetQueryExits(queryExitsFlowThrough, queryExitsEnding);

                        // Get the top guy ID
                        WayPointID topGuyID = nextWP->GetId();

                        // JOINMERGE
                        // If join waypoint, add the other 3 nodes
                        // join_merge, join_writer, cleaner
                        // JOINMERGE
                        if (wp->GetType() == JoinWaypoint) {
                            //string name = ID.getName();
                            //name += "_merge";
                            //WayPointID joinmergeID(name.c_str());
                            string namew = ID.getName();
                            namew += "_writer";
                            WayPointID joinwriterID(namew.c_str());
                            if (!isTerminating) {
                                queryExitsFlowThrough.MoveToStart ();
                                while (queryExitsFlowThrough.RightLength ()) {
                                    QueryExit qe = queryExitsFlowThrough.Current ();
                                    //g.AddLink (joinmergeID, topGuyID, qe);
                                    g.AddLink (joinwriterID, topGuyID, qe); // Is this also with same qe?
                                    QueryID nullOne;
                                    QueryExit writerExit (nullOne, joinwriterID);
                                    g.AddLink (cleanerID, joinwriterID, writerExit);
                                    queryExitsFlowThrough.Advance ();
                                }
                            } else {
                                queryExitsEnding.MoveToStart ();
                                while (queryExitsEnding.RightLength ()) {
                                    QueryExit qe = queryExitsEnding.Current ();
                                    //g.AddLink (joinmergeID, topGuyID, qe);
                                    g.AddLink (joinwriterID, topGuyID, qe); // Is this also with same qe?
                                    QueryID nullOne;
                                    QueryExit writerExit (nullOne, joinwriterID);
                                    g.AddLink (cleanerID, joinwriterID, writerExit);
                                    queryExitsEnding.Advance ();
                                }
                            }
                        }

                        if (!isTerminating) {
                            queryExitsFlowThrough.MoveToStart ();
                            while (queryExitsFlowThrough.RightLength ()) {
                                QueryExit qe = queryExitsFlowThrough.Current ();
                                g.AddLink (ID, topGuyID, qe);
                                queryExitsFlowThrough.Advance ();
                            }
                        } else {

                            queryExitsEnding.MoveToStart ();
                            while (queryExitsEnding.RightLength ()) {
                                QueryExit qe = queryExitsEnding.Current ();
                                g.AddLink (ID, topGuyID, qe);
                                queryExitsEnding.Advance ();
                            }
                        }
                    }
                }
            }
            if (!bfs.reached(n)) {
                bfs.addSource(n);
                bfs.start();
            }
    }
#ifdef DEBUG
    g.Print();
#endif
    ofstream file1;
        file1.open("DOT1.dot");
           g.PrintDOT(file1);
    file1.close();
    g.swap(rez);
}


string LemonTranslator::CompileCode(string dir){
    // let's make the call string for generate.sh, which takes the
    // directory itself as a parameter
  string call = "./generate.sh " + dir;// + " 1>&2";
#ifdef DEBUG
    cout << call << "\n";
#endif
    // do the system call, generate.sh must return 0 on success
    int sysret = execute_command(call.c_str());
    if (sysret != 0){
        perror("LemonTranslator compile");
        FATAL("Unable to do the code generation on directory %s!",
            dir.c_str());
    }

    return dir+"/Generated.so";
}

string  LemonTranslator::GenerateCodeJSON( string dir ) {
    string call = "./processJSON.sh " + dir + " ./query.json";
    int sysret = execute_command(call.c_str());

    if( sysret != 0 ) {
        perror("LemonTranslator compile");
        FATAL("Unable to perform code generation on directory %s!",
                dir.c_str());
    }

    return dir + "/Generated.so";
}


bool LemonTranslator::AnalyzeAttUsageBottomUp(QueryIDSet queries)
{
    // Create a NodeMap which is kind of hash to store int for each corrosponding graph node
    ListDigraph::NodeMap<int> order(graph);
    // sort topologically
    topologicalSort(graph, order);
    // create a reverse map, sort order -> node
    map<int, ListDigraph::Node> sortedOrder; // we need reverse mapping (order, node)
    for (ListDigraph::ArcIt a(graph); a != INVALID; ++a) {
        sortedOrder[order[graph.source(a)]] = graph.source(a);
        sortedOrder[order[graph.target(a)]] = graph.target(a); // not to miss corner nodes
    }

    // Traversal must start from bottom-most point in toppological sorted order
    map<int, ListDigraph::Node>::const_iterator it = sortedOrder.begin();
    // assert if we are not starting from bottom node (which is kind of virtual node)
    assert(it->second == bottomNode);
    // Iterate on all nodes in topo sort order
    for (;it != sortedOrder.end(); ++it) {
        // get the node from the sort map
        ListDigraph::Node node = it->second;
        // get the actual waypoint from another map
        LT_Waypoint* thisWP = nodeToWaypointData[node];
        // If we found some WP, start processing (will be null for virtual bottom node and virtual top node)
        if (thisWP) {
            // get the attributes which we need to send up
            map<QueryID, set<SlotID> > attributes;
            if ( !thisWP->PropagateUp(attributes) ){
                cout << "Wrong call to PropagateUp";
                return false;
            }
            // Pass the attributes to all outgoing connected waypoint nodes
            for (ListDigraph::OutArcIt arc(graph, node); arc != INVALID; ++arc) {
                // get the next up node
                ListDigraph::Node next = graph.target(arc);
                // get the waypoint from map
                LT_Waypoint* nextWP = nodeToWaypointData[next];

                // pass the attributes to above waypoint. Terminating edges are right of join and
                // edges going to print waypoints. Rest are non terminating
                // Right now we dont have terminating functions for scanner and textloader, and we dont need them basically
                // so explicit check to make sure correct functions are called if we have terminating edges from textloader to scanner
                // actually textLoader check can be removed because it is at lowest end and it can never be nextWaypoint pointer
                if (nextWP) {
                    //if ((thisWP->GetType() == ScannerWaypoint || thisWP->GetType() == TextLoaderWaypoint) || !terminatingArcMap[arc])
                    if ((nextWP->GetType() == ScannerWaypoint || nextWP->GetType() == TextLoaderWaypoint) || !terminatingArcMap[arc])
                        nextWP->ReceiveAttributes(attributes); // Will copy only if have those queries
                    else
                        nextWP->ReceiveAttributesTerminating(attributes); // Will copy only if have those queries
                }
            }
        }
    }
    // make sure we end at virtual top node
    assert(it->second == topNode);
    return true;
}


bool LemonTranslator::AnalyzeAttUsageTopDown(QueryID query,
        ListDigraph::Node node,
        QueryExit exitWP,
        set<SlotID>& propagated) {

    //exitWP.Print(); cout << flush;
    assert(exitWP.IsValid());
    // Proceed only if we are not virtual nodes
    if (node != topNode && node != bottomNode)
    {
        LT_Waypoint* thisWP = nodeToWaypointData[node];

        // If waypoint do not have query, just return from this recursion hierarchy
        if (thisWP && !thisWP->DoIHaveQueries(query)) return false;

        // This will be used in case we find terminating arc if this is terminating node
        // Because we need to keep on updating queryExits if we find some terminating arc
        // on the way down
        QueryExit qe(query, thisWP->GetId());

        // scan incoming edges to find next below nodes because graph is created in directed way upwards
        for (ListDigraph::InArcIt arc(graph, node); arc != INVALID; ++arc) {
            // get the down guy
            ListDigraph::Node next = graph.source(arc);
            //LT_Waypoint* wp = nodeToWaypointData[next];
            // see if its terminating edge
            bool isTerminating = terminatingArcMap[arc];

            set<SlotID> propDown;
            if (!isTerminating) {
                if (!thisWP->PropagateDown(query, propagated, propDown, exitWP) ){
                    cout << "\nWrong call to PropagateDown " << thisWP->GetType();
                    return false;
                }
            }
            else {
                QueryExit queryExit;
                queryExit.copy(qe);
                if ( !thisWP->PropagateDownTerminating(query, propagated, propDown, queryExit) ){
                    cout << "\nWrong call to PropagateDownTerminating " << thisWP->GetType();
                    return false;
                }
            }
            // recursive call
            QueryExit queryExit(exitWP);
            if (isTerminating)
                queryExit.copy(qe);
            AnalyzeAttUsageTopDown(query, next, queryExit, propDown);
        }
        // Removed scanner code from here
    }

    return true;
}

void LemonTranslator::AddPreamble(ostream &out) {

    // get the current time in a nice, ascii form
    time_t rawtime = time(NULL);
    tm *timeinfo = localtime(&rawtime);

    // add the preamble with some debugging comments
    out << "dnl # CODE GENERATED BY DATAPATH ON " << asctime(timeinfo) << endl;
    out << "include(Modules.m4)" << endl;
    out << "M4_CODE_GENERATION_PREAMBLE" << endl << endl;
}

void LemonTranslator::GetAccumulatedLHSRHS(set<SlotID>& LHS, set<SlotID>& RHS, QueryIDSet& queries) {

    Bfs<ListDigraph> bfs(graph);
    bfs.init();
    for (ListDigraph::NodeIt n(graph); n != INVALID; ++n) {
        if (n != topNode && n != bottomNode) {
                LT_Waypoint* wp = nodeToWaypointData[n];
                if (wp->GetType() == JoinWaypoint) {
                    wp->GetAccumulatedLHSRHS(LHS, RHS, queries);
                }
        }
        if (!bfs.reached(n)) {
            bfs.addSource(n);
            bfs.start();
        }
    }
}

bool LemonTranslator::GenerateMessages()
{
    return true;
}

void LemonTranslator::FillTypeMap(std::map<WayPointID, WaypointType>& typeMap) {
    Bfs<ListDigraph> bfs(graph);
    bfs.init();
    for (ListDigraph::NodeIt n(graph); n != INVALID; ++n) {
        if (n != topNode && n != bottomNode) {
                LT_Waypoint* wp = nodeToWaypointData[n];
                typeMap[wp->GetId()] = wp->GetType();
        }
        if (!bfs.reached(n)) {
            bfs.addSource(n);
            bfs.start();
        }
    }
}

Json::Value LemonTranslator::GetJson() {
    Json::Value data(Json::objectValue);

    data[J_JOB_ID] = jobID;
    data[J_HEADER] = headers;

    // Nodes
    {
        Json::Value nodes(Json::arrayValue);

        Bfs<ListDigraph> bfs(graph);
        bfs.init();
        for (ListDigraph::NodeIt n(graph); n != INVALID; ++n) {
            if (n != topNode && n != bottomNode) {
                LT_Waypoint* wp = nodeToWaypointData[n];
                nodes.append(wp->GetJson());
            }
            if (!bfs.reached(n)) {
                bfs.addSource(n);
                bfs.start();
            }
        }

        // This algorithm finds the nodes starting from the end (for some reason), so
        // reverse the order.
        for( int i = 0; i < (nodes.size() / 2); ++i ) {
            int j = nodes.size() - 1 - i;
            Json::Value & nodeI = nodes[i];
            Json::Value & nodeJ = nodes[j];
            nodeI.swap(nodeJ);
        }

        // Add cleaner waypoint
        nodes.append(GetJsonCleaner());

        data[J_WAYPOINTS] = nodes;
    }

    // Edges
    {
        Json::Value edges(Json::arrayValue);

        // Traverse full graph step by step
        Bfs<ListDigraph> bfs(graph);
        bfs.init();
        for( ListDigraph::NodeIt n(graph); n != INVALID; ++n ) {
            if( n != topNode && n != bottomNode ) {
                LT_Waypoint * wp = nodeToWaypointData[n];
                string sourceName = wp->GetWPName();

                // Find all the outlinks for the node
                for( ListDigraph::OutArcIt arc(graph, n); arc != INVALID; ++arc) {
                    ListDigraph::Node next = graph.target(arc);
                    bool isTerminating = terminatingArcMap[arc];
                    if( next != topNode && next != bottomNode ) {
                        LT_Waypoint * nextWP = nodeToWaypointData[next];
                        string destName = nextWP->GetWPName();

                        WayPointID destID = nextWP->GetId();

                        QueryExitContainer queryExitsFlowThrough;
                        QueryExitContainer queryExitsEnding;
                        nextWP->GetQueryExits(queryExitsFlowThrough, queryExitsEnding);

                        // Get a list of queries this arc corresponds to
                        QueryManager & qm = QueryManager::GetQueryManager();

                        Json::Value queries(Json::arrayValue);
                        QueryIDSet queriesCovered;

                        // Only one of the following should actually have stuff
                        // in it.
                        FOREACH_TWL(qe, queryExitsFlowThrough) {
                            QueryID qID = qe.query;
                            if( !queriesCovered.Overlaps(qID) ) {
                                queriesCovered.Union(qID);
                                string qName;
                                qm.GetQueryName(qID, qName);
                                queries.append(qName);
                            }
                        } END_FOREACH;

                        FOREACH_TWL(qe, queryExitsEnding) {
                            QueryID qID = qe.query;
                            if( !queriesCovered.Overlaps(qID) ) {
                                queriesCovered.Union(qID);
                                string qName;
                                qm.GetQueryName(qID, qName);
                                queries.append(qName);
                            }
                        } END_FOREACH;

                        Json::Value edge(Json::objectValue);
                        edge["source"] = sourceName;
                        edge["dest"] = destName;
                        edge["terminating"] = isTerminating;
                        edge["queries"] = queries;

                        edges.append(edge);
                    }
                }
            }
        }

        data[J_EDGES] = edges;
    }

    AttributeManager& am = AttributeManager::GetAttributeManager();
    Json::Value attrs;
    am.GenerateJSON(attrs);
    data[J_ATTRIBUTES] = attrs;

    return data;
}

Json::Value LemonTranslator::GetJsonCleaner() {
    set<SlotID> LHS;
    set<SlotID> RHS;
    QueryIDSet queries;
    GetAccumulatedLHSRHS(LHS, RHS, queries);

    AttributeManager & am = AttributeManager::GetAttributeManager();
    QueryManager & qm = QueryManager::GetQueryManager();

    Json::Value info(Json::objectValue);
    info["LHS"] = Json::Value(Json::arrayValue);
    info["RHS"] = Json::Value(Json::arrayValue);
    info["query_names"] = Json::Value(Json::arrayValue);
    info["queries"] = (Json::Int64) queries.GetInt64();

    // Get list of queries covered
    QueryIDSet qCopy = queries;
    while( !qCopy.IsEmpty() ) {
        QueryID x = qCopy.GetFirst();
        std::string qName;
        qm.GetQueryName(x, qName);
        info["query_names"].append(qName);
    }

    // Get list of attributes on LHS
    for( auto slot : LHS ) {
        info["LHS"].append(am.GetAttributeName(slot));
    }

    // Get list of attributes on RHS
    for( auto slot : RHS ) {
        info["RHS"].append(am.GetAttributeName(slot));
    }

    info[J_TYPE] = JN_CLEANER_WP;
    info[J_NAME] = "Cleaner";

    return info;
}
