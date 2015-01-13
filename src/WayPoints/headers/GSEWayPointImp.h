// Copyright 2013 Tera Insights, LLC. All Rights Reserved.

#ifndef GSE_WAY_POINT_IMP
#define GSE_WAY_POINT_IMP

#include <list>
#include <unordered_map>
#include <utility>
#include <string>

#include "ID.h"
#include "SerializeJson.h"
#include "GPWayPointImp.h"
#include "GLAData.h"
#include "GLAHelpers.h"
#include "Constants.h"
#include "ServiceData.h"

class GSEWayPointImp : public GPWayPointImp {

    /***** Typedefs *****/
    using StateList = std::list<GLAState>
    using QueryToStateList = std::unordered_map<QueryID, StateList>;
    using RequestQueue = std::list<ServiceData>;

    using ServiceMap = std::unordered_map<std::string, QueryID>;

    /***** Data Members *****/

    // States used to serve requests
    QueryToStateList gseStates;

    // Mapping from service name to the query that fulfills requests for that
    // service
    ServiceMap serviceMap;

    // List of requests awaiting execution
    RequestQueue requests;

    // Preprocessing stage
    // Creates any generated states needed
    QueryIDSet queriesToPreprocess;
    QueryIDSet queriesPreprocessing;

    // Processing (serving queries)
    // Additional states are constructed as needed
    QueryIDSet queriesProcessing;

    // Completed (no longer serving queries)
    QueryIDSet queriesCompleted;
};

#endif // GSE_WAY_POINT_IMP
