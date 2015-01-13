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
/** for Emacs -*- c++ -*- */
private:
    /* Internal struct so we can form a map to values */
    struct QueryParameter {
        QueryID query;
        std::string param;

        QueryParameter(QueryID _query, std::string& _param):query(_query), param(_param){}
        QueryParameter(){}
        ~QueryParameter(){}

        bool operator <(const QueryParameter& other) const {
            return (query<other.query) || (query==other.query && param<other.param);
        }
    };

    typedef std::map<QueryParameter, std::string> QueryParameterMap;

    QueryIDSet queries; // set of queries mentioned in paramters

    // Map between integer param names and values
    std::map<std::string, int> intParams;

    // Map between double param names and values
    std::map<std::string, double> doubleParams;

    // Map between string param names and values
    std::map<std::string, std::string> stringParams;

    // Map between query,parameter to values
    QueryParameterMap queryParameterMap;

    // Waypoint ID
    WayPointID wid;

    // Waypoint type
    WaypointType wtype;

    // Disallow assignment and copy constructors
    SymbolicWaypointConfig(const SymbolicWaypointConfig &s);
    SymbolicWaypointConfig &operator=(const SymbolicWaypointConfig &w);
