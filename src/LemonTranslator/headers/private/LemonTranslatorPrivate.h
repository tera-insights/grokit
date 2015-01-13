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
// Internal datastructures maintained by LemonTranslator

private:

    // keep track of old and new queries
    QueryIDSet newQueries;
    QueryIDSet deleteQueries; // queries deleted (need to inform execution engine

    WayPointID cleanerID; // this is the cleaner waypoint

    std::string error; // the error message

    ListDigraph graph;

    // A dummy node above all print waypoints to ease traversal
    ListDigraph::Node topNode;

    // A dummy node below all scanner waypoints to ease traversal
    ListDigraph::Node bottomNode;

    // Graph is made of WayPointID, so keep WayPointID std::map for each node
    //ListDigraph::NodeMap<WayPointID> nodeMap;

    std::map<WayPointID, ListDigraph::Node> IDToNode;

    // Arc property to indicate if it is a terminating or non-terminating edge
    ListDigraph::ArcMap<bool> terminatingArcMap;

    // map from graph node to waypoint data
    std::map<ListDigraph::Node, LT_Waypoint*> nodeToWaypointData;

    // map from QueryID to Node/WayPointID that specifies the starting point for each query
    // from where top to bottom analysis has to be started
    std::map<QueryID, ListDigraph::Node> queryToRootMap;

    TaskList tasks;

    // In case new Run, then only return config to generate code, else not
    bool isNewRun;

    // Stores header commands in a JSON array
    Json::Value headers;

    // AUX FUNCTIONS
    bool AddGraphNode(WayPointID WPID, WaypointType type, LT_Waypoint*& WP);

    bool GetWaypointAttr(WayPointID WPID, SlotContainer& atts, std::set<SlotID>& attr, LT_Waypoint*& WP);

    // Functions to recursively analyse graph per query and update the waypoints
    // return false with error message if encounter some problem (graph incomplete etc)
    bool AnalyzeAttUsageTopDown(QueryID query,
            ListDigraph::Node node,
            QueryExit exitWP, // the waypoint where data deleted
            std::set<SlotID>& propagated);

    bool AnalyzeAttUsageBottomUp(QueryIDSet query);


    // translate from query to queryExit
    QueryExit QueryToQueryExit(TableScanID scanner, QueryID query);


    // M4 generation Aux functions
    void AddPreamble(std::ostream& out);

    void PopulateGraph(DataPathGraph& rez);

    void PopulateWaypoints(SymbolicWPConfigContainer&);

    void PopulateScanners(ScannerConfigContainer& scanners);

    void PlotGraph(std::string dir);


