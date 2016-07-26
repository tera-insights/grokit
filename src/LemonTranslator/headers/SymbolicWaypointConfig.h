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
#ifndef _SYMBOLICWAYPOINTCONFIG_H
#define _SYMBOLICWAYPOINTCONFIG_H

#include "ID.h"
#include "Errors.h"
#include "Bitstring.h"
#include "TwoWayList.h"
#include "TwoWayList.cc"

#include <map>
#include <string>
#include <istream>

#define NUM_WAYPOINT_TYPES 4

/* Possible types of waypoints we can have in the system. */
enum WaypointType {
    InvalidWaypoint,
    SelectionWaypoint,
    JoinWaypoint,
    PrintWaypoint,
    ScannerWaypoint,
    TextLoaderWaypoint,
    GLAWaypoint,
    GTWaypoint,
    GISTWayPoint,
    GIWayPoint,
    CacheWaypoint,
    CompactWaypoint,
    ClusterWaypoint
};

struct WPTypeTranslator {
    const char* typeName;
    WaypointType type;
};

extern WPTypeTranslator nameofTypes[NUM_WAYPOINT_TYPES];

/** This class encapsulates the set of parameters given to a waypoint
 * in order to configure it.
 *
 * Essentially, this structure contains a mapping between parameter
 SymbolicWaypointConfig	* names and values, in a manner similar to that maintained by the
 * Catalog. Additionally, the waypoint type and ID have to be
 * kept.
 *
 * To allow easy maagement of information, we allow parameters to be specified/query
 * as well. In this way, we do not have to be overly conderned with the M4 format
 * Each waypoint configuration object will know what parameters have to be deined
 * and how to use them to generate the m4 code
 *
 * This object is trully generic and provides no error checking
 * whatsoever. The checking is done by the specific waypoint code.
 *
 * NOTE: only strings are supported as query specific parameters A
 * facility to extract the set of queries mentioned in the
 * specification is also provided.
 *
 * NOTE2: The meaning of each parameter and the list of parameters
 * expected is defined in the file CodeGenerator/"Waypointname"CG.h
 *
 * This object is swappable.
 **/
class SymbolicWaypointConfig {

#include "SymbolicWaypointConfigPrivate.h"

    public:
        /* Default constructor */
        SymbolicWaypointConfig();

        /* Constructor with waypoint type and waypoint id */
        SymbolicWaypointConfig(WaypointType type, WayPointID id);

        void SetType(const char* type);

        /* Constructor from stream containing the flat description (WaypiontFile) */
        SymbolicWaypointConfig(FILE* fromMe);

        /* Destructor */
        virtual ~SymbolicWaypointConfig() {}

        /** XML constructor to be added in the future*/

        /* Sets the value of an integer parameter */
        void SetIntParam(std::string name, int value);

        /* Sets the value of a double parameter */
        void SetDoubleParam(std::string name, double value);

        /* Sets the value of a string parameter */
        void SetStringParam(std::string name, std::string value);

        /* Sets the value of a query specific string parameter */
        void SetStringParam(QueryID query, std::string name, std::string value);

        /* Extract the set of queries mentioned in config */
        QueryIDSet GetQueries(void);

        /* Returns the value of an integer parameter */
        int GetIntParam(std::string name);

        /* Returns the value of a double parameter */
        double GetDoubleParam(std::string name);

        /* Returns the value of a string parameter */
        std::string GetStringParam(std::string name);

        /* Same but for query specifig string parameter */
        std::string GetStringParam(QueryID query, std::string name);

        /* Returns the waypoint type */
        WaypointType GetType();

        /* Returns the waypoint ID */
        WayPointID GetID();

        /* Swaps the contents of this and another object */
        void swap(SymbolicWaypointConfig &who);
};

// container of Symbolic Waypoint configurators
typedef TwoWayList<SymbolicWaypointConfig>  SymbolicWPConfigContainer;


inline QueryIDSet SymbolicWaypointConfig::GetQueries(void){
    return queries;
}

inline void SymbolicWaypointConfig::SetIntParam(std::string name, int value) {
    intParams[name] = value;
}

inline void SymbolicWaypointConfig::SetDoubleParam(std::string name, double value) {
    doubleParams[name] = value;
}

inline void SymbolicWaypointConfig::SetStringParam(std::string name, std::string value) {
    stringParams[name] = value;
}

inline void SymbolicWaypointConfig::SetStringParam(QueryID query, std::string name, std::string value){
    queries.Union(query); // to know we set this query
    queryParameterMap[QueryParameter(query,name)]=value;
}

inline WaypointType SymbolicWaypointConfig::GetType() {
    return(wtype);
}

inline WayPointID SymbolicWaypointConfig::GetID() {
    return(wid);
}

#endif // _SYMBOLICWAYPOINTCONFIG_H
