//
//  Copyright 2012 Alin Dobra and Christopher Jermaine
//  Copyright 2013 Tera Insight
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
#ifndef _LEMON_TRANSLATOR_H
#define _LEMON_TRANSLATOR_H

#include <set>
#include <vector>

#include <cstddef>

#include "Catalog.h"
#include "ID.h"
#include "QueryExit.h"
#include "DataPathGraph.h"
#include "SymbolicWaypointConfig.h"
#include "WayPointConfigureData.h"
#include "ScannerConfig.h"
#include <lemon/list_graph.h>
#include "ContainerTypes.h"
#include "JsonAST.h"
#include "Tasks.h"

using namespace lemon;
class LT_Waypoint;

/** This is the interface to the Lemon based Translator */
class LemonTranslator {
    public:

#include "LemonTranslatorPrivate.h"

    private:
        bool batchMode;

        std::string jobID;

    public:
        enum JoinType { Join_EQ, Join_IN, Join_NOTIN };

        LemonTranslator(bool batchMode);

        // return a text error message
        std::string GetErrorMessage();

        // remove all info about a query from the system
        // the list needs to be made available to the Scanners as well
        bool DeleteQuery(QueryID query);

        bool AddQuery(std::string qName, QueryID query);

        void AddHeader( Json::Value val );

        void AddTask( Task & );

        void SetJobID( const std::string & );

        /************ Adding new waypoints ***************/
        // Check error message and return status of all functions returning bool below
        // It is important not to corrupt graph by not ignoring error status
        bool AddScannerWP(WayPointID scanner,
            std::string relName,
            SlotContainer& attributes /** atrributes that scanner can read */);

        bool AddTextLoaderWP(WayPointID loaderWP,
                SlotContainer& attributes, /* atrributes that the loader can read */
                Json::Value& info /* information on text loader in JSON */
                );

        bool AddGIWP( WayPointID giWP,      // ID of the waypoint
                SlotContainer& attributes,  // Attributes to be read, in the order the GI returns them
                Json::Value& info /* information on text loader in JSON */
                );

        bool AddSelectionWP(WayPointID selWP);
        bool AddJoinWP(WayPointID joinWP, SlotContainer& LHS_att, Json::Value& info);
        bool AddAggregateWP(WayPointID aggWP);
        bool AddPrintWP(WayPointID printWP);
        bool AddGLAWP(WayPointID glaWP);
        bool AddGTWP(WayPointID gfWP);
        bool AddGISTWP(WayPointID gistWP);
        bool AddCacheWP(WayPointID wpID);
        bool AddCompactWP(WayPointID wpID);
        bool AddClusterWP(WayPointID wpID,
            std::string relation, SlotID cAtt, QueryID query);

        // Adding Edges to the graph (terminating or non-terminating)
        // The edge direction should be provided correctly, always bottom to top
        // (bottom, top)
        bool AddEdge(WayPointID start, WayPointID end);
        bool AddTerminatingEdge(WayPointID start, WayPointID end);

        bool AddEdgeFromBottom(WayPointID WPID);
        bool AddEdgeToTop(WayPointID WPID);

        /******* Adding information for queries (per query) ****************/

        bool AddCaching(WayPointID wp, QueryID query);
        bool AddCompact(WayPointID wp, QueryID query);

        // These functions might not be apropriate for the type
        // of waypoint defined earlier. If that is the case, false is
        // returned and error is produced

        // Selection
        bool AddFilter(WayPointID wp, QueryID query, SlotContainer& atts,
                std::vector<WayPointID> reqStates,
                Json::Value& info);

        // Selection. Need a list of these
        bool AddSynthesized(WayPointID wp, QueryID query,
                SlotID attribute /** this is the synthesiezd attribute*/,
                SlotContainer& atts, Json::Value& expr);
        // Selection
        //    bool AddSynthesized(WayPointID wp, QueryID query,
        //                                            SlotID attribute, /** this is the synthesiezd attribute*/
        //                                            SlotContainer& atts, /** attributes in expression */
        //                                            std::string expr
        //                                            );


        //GLA, one per query basis
        bool AddGLA(WayPointID wp, QueryID query,
                SlotContainer& resultAtts, /*list of attributes produced as the result */
                SlotContainer& atts,
                std::vector<WayPointID>& reqStates,
                Json::Value & info
                );

        bool ReturnAsState(WayPointID wp, QueryID query);

        // GIST, one per query basis
        bool AddGIST( WayPointID wp, QueryID query,
                SlotContainer& resultAtts,
                std::vector<WayPointID> reqStates,
                Json::Value& info
                );

        // GT, one per query basis
        bool AddGT(WayPointID wp, QueryID query,
                SlotContainer& resultAtts,  // list of attributes produced as the result
                SlotContainer& atts,
                std::vector<WayPointID> reqStates,
                Json::Value& info
                );

        // Selection, Join. Queries added one by one
        bool AddBypass(WayPointID wp, QueryID query);

        // Join
        bool AddJoin(WayPointID wp, QueryID query, SlotContainer& RHS_atts /* right hand side attributes */,
                LemonTranslator::JoinType jType, Json::Value& info);

        // Print. Need a list of these
        bool AddPrint(WayPointID wp, QueryID query, SlotContainer& atts,
                Json::Value& info
                );

        bool AddWriter(WayPointID wpID, QueryID query, SlotToSlotMap& storeMap);

        bool AddScannerRange(WayPointID wpID, QueryID query, int64_t min, int64_t max);
        bool AddScanner(WayPointID wpID, QueryID query);

        /****Interface for the rest of the system***************/

        void ClearAllDataStructure();

        // All the queries in the argument should be ready to go
        // and the translator should produce the instructions ..
        // dir is the directory where the results are placed
        bool Run(QueryIDSet whatQueries);

        // Generate Messages for all nodes
        bool GenerateMessages();

        // compile the code
        // Params:
        //   dir: the directory where the code is
        //   objects: list of objects that need to be included
        //   RETURNS: name of the library created
        std::string CompileCode(std::string dir);
        std::string GenerateCodeJSON(std::string dir);

        // Function to plot the query plan graph in a dot file so it can be plotted
        void PrintDOT(std::ostream& out);

        // function to compute the configuration message for the rest of the system
        // newQueries will contain the new queries (since last call)
        bool GetConfig(std::string, QueryExitContainer& newQueries, DataPathGraph&, WayPointConfigurationList&, TaskList &);

        void PopulateWayPointConfigurationData(WayPointConfigurationList& myConfigs);

        void FillTypeMap(std::map<WayPointID, WaypointType>& typeMap);

        void GetAccumulatedLHSRHS(std::set<SlotID>& LHS, std::set<SlotID>& RHS, QueryIDSet& queries);

        Json::Value GetJson();
        Json::Value GetJsonCleaner();
};


#endif // _LEMON_TRANSLATOR_H
