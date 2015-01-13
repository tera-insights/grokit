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
#ifndef _QUERYEXIT_H_
#define _QUERYEXIT_H_

#include "ID.h"
#include "TwoWayList.h"
#include "TwoWayList.cc"
#include "EfficientMap.h"
#include "EfficientMap.cc"

/** QueryExits specify at what WayPoint the information about a query
 * is consumed completedly. The QueryExits make sense only in the
 * context of a given TableScan.

 This class is designed to work with both the swapping paradigm and
 with the standard stl. The class is about as lightweight as an id
 so it can be passed by value.
 */

struct QueryExit {
    QueryID query; // the query this chunk belongs to
    WayPointID exit;  // the place it is exiting

    // constructor
    QueryExit(QueryID _query, WayPointID _exit):query(_query),exit(_exit){ }
    QueryExit(const QueryExit &fromMe):query(fromMe.query),exit(fromMe.exit) {}
    QueryExit(){}

    // destructor
    virtual ~QueryExit (){}


    void swap (QueryExit &withMe);
    void CopyFrom(const QueryExit &withMe);
    void copy(const QueryExit &withMe){ CopyFrom(withMe); }
    bool IsEqual (QueryExit &withMe) const;
    bool operator ==(QueryExit& other) const {return IsEqual (other);}
    bool operator <(const QueryExit& other) const;
    bool LessThan(const QueryExit& other) const;
    void Print ();
    std::string GetStr ();
    bool IsValid(void);

    void toJson( Json::Value & ) const;
    void fromJson( const Json::Value & );
};

// list of (query, exit) pairs
typedef TwoWayList<QueryExit> QueryExitContainer;

// map between queryIds and slots they use
typedef EfficientMap < QueryExit, SlotContainer > QueryExitToSlotsMap;

// function to compute QueryIDSet (bitstring) from query exits
// Essentially the waypoint exit is ignored and queries are
// represented by the more efficient QueryIDSets
QueryIDSet QueryExitsToQueries(QueryExitContainer& qeSet);

/****** INLINE FUNCTIONS *********/

inline
void QueryExit :: toJson( Json::Value & dest ) const {
    dest = Json::Value(Json::objectValue);
    ToJson(query, dest["query"]);
    ToJson(exit, dest["exit"]);
}

inline
void ToJson( const QueryExit & src, Json::Value & dest ) {
    src.toJson(dest);
}

inline
void QueryExit :: fromJson( const Json::Value & src ) {
    FromJson(src["query"], query);
    FromJson(src["exit"], exit);
}

inline
void FromJson( const Json::Value & src, QueryExit & dest ) {
    dest.fromJson(src);
}

inline QueryIDSet QueryExitsToQueries(QueryExitContainer& qeSet){
    QueryIDSet queries;

    FOREACH_TWL(q, qeSet){
        queries.Union(q.query);
    }END_FOREACH

    return queries;
}

inline
bool QueryExit::operator <(const QueryExit& other) const{
    return (query<other.query || (query==other.query && exit<other.exit));
}

inline void QueryExit::Print () {
    query.Print ();
    std::cout << " ";
    exit.Print ();
}

inline std::string QueryExit::GetStr () {
    std::string str;
    str += query.GetStr ();
    str += " ";
    str += exit.getName ();
    return str;
}

inline
bool QueryExit::LessThan(const QueryExit& other) const{
    return (query<other.query || (query==other.query && exit<other.exit));
}


inline
void QueryExit::swap (QueryExit &withMe){
    query.swap(withMe.query);
    exit.swap(withMe.exit);
}

inline
void QueryExit::CopyFrom (const QueryExit &fromMe){
    query=fromMe.query;
    exit=fromMe.exit;
}

inline
bool QueryExit::IsEqual (QueryExit &withMe) const {
    return (query==withMe.query && exit==withMe.exit);
}

inline
bool QueryExit::IsValid(void){ return (!query.IsEmpty() && exit.IsValid()); }

#endif // _QUERYEXIT_H_
