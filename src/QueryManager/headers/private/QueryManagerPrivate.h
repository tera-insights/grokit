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
private:
    // Mutex for the QueryManger
    pthread_mutex_t queryManagerMutex;

    //the slots for the queries, 0 not occupied, 1 occupied.
    int* slot;

    // This is a wrapper for the query information
    struct QueryWrapper
    {
        // This stores the queryid
        QueryID myID;
        // This is the bitmap index for the query
        int myMap;
        //constructor
        QueryWrapper(QueryID QID,int myMap);
        //destructor
        ~QueryWrapper();

    };

    typedef std::map<std::string,QueryWrapper*> QueriesNameInfoMap;

    // Map for holding key as query name and value as QueryWrapper
    QueriesNameInfoMap mapQueriesNameInfo;

    typedef std::map<QueryID, std::string> IDToName;
    IDToName idToName; // map to provide name when given id

    typedef std::map<std::string, QueryID> NameToID;
    NameToID nameToID;

    typedef std::set<std::string> QueryNameSet;
    QueryNameSet deletedQueries;

    //this returns the next empty slot, -1 if no empty slot left
    int nextEmptySlot();

    // Singleton instance of QueryManager.
    static QueryManager *instance;

    // The constructor is hidden here.
    QueryManager();

    // Used internally to get information about given query whose name is given
    QueryWrapper* getQueryInfo(std::string queryName);

    // Used internally to get information about given query whose ID is given
    QueryWrapper* getQueryInfo(QueryID queryID );
