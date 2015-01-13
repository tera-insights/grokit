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
#include "QueryManager.h"
#include "Logging.h"
#include "Errors.h"
#include "Bitstring.h"

#include <iostream>
#include <string>

using namespace std;

// the instance is default NULL.
QueryManager *QueryManager::instance = NULL;


// returns the instance.
QueryManager &QueryManager::GetQueryManager() {

    // does it exist? if not, construct it.
    if (instance == NULL)
        instance = new QueryManager();

    // return it.
    return(*instance);
}

//the constructor
QueryManager :: QueryManager() {
    //initialize the mutex
    pthread_mutex_init (&queryManagerMutex, NULL);

    slot = new int[Bitstring::MaxSize()];

    //initialize the slots into 0
    for(unsigned int i=0; i<Bitstring::MaxSize(); ++i)
        slot[i]=0;

    //prepare all the spaces for the queries;
    //myQueries = new vector<QueryManager:: QueryWrapper*>;
}

//the destructor
QueryManager :: ~QueryManager() {
    delete [] slot;

    //destroy the mutex
    pthread_mutex_destroy (&queryManagerMutex);

    //all the queries must have finished before the query manager is killed
    int size = mapQueriesNameInfo.size();

    FATALIF(size != 0, "QueryManager::There are still queries running. Cannot shutdown.");

    // delete all entries in the map
    mapQueriesNameInfo.clear();
}


//this initialize the QuerWrapper
QueryManager :: QueryWrapper :: QueryWrapper(QueryID QID, int _myMap) {
    myID=QID;
    myMap=_myMap;
}


//this is the destructor for the QueryWrapper
QueryManager :: QueryWrapper :: ~QueryWrapper() {}


//the function for the next
int QueryManager :: nextEmptySlot() {
    //find the first place that slot[i]==0
    for(unsigned int i=0; i<Bitstring::MaxSize(); ++i)
    {
        if (slot[i]==0)
        {
            slot[i] =1; //now occupied
            return i;
        }
    }
    return -1;
}

bool QueryManager :: AddNewQuery(string queryName, QueryID& returnMe) {
    //get the mutex
    pthread_mutex_lock (&queryManagerMutex);

    QueriesNameInfoMap::iterator mapItr;
    mapItr = mapQueriesNameInfo.find(queryName);
    bool retVal = true;

    //check if query is already present in map or not
    if(mapItr == mapQueriesNameInfo.end())
    {
        // proceed if query is not already present

        //get the mapping for the ID
        int mapping = nextEmptySlot();

        //if no empty slot, exit with error(0)
        FATALIF(mapping == -1,"QueryManager: No more empty slot left. \
                The number of queries running reaches the upper bound.");

        //get new QueryID
        returnMe.Empty(); // to make sure nothing is left
        returnMe.AddMember(mapping);

        // now create query information wrapper instance
        QueryWrapper* queryWrapper = new QueryWrapper(returnMe,mapping);
        // and add this information to the map
        mapQueriesNameInfo[queryName] = queryWrapper;

        QueryID queryIdToInsert = returnMe;
        idToName.insert(pair<QueryID, string>(queryIdToInsert,queryName));

        nameToID[queryName] = queryIdToInsert;
    }
    else
    {
        // else give warning
        string warning = "Cannot add new query with name " + queryName + ". \
                          Query with same query name already exists!";
        WARNING("%s", warning.c_str());
        retVal = false;
    }
    //return the mutex
    pthread_mutex_unlock (&queryManagerMutex);

    LOG_ENTRY_P(2, "Query %s inserted.", queryName.c_str());

    return retVal;
}

bool QueryManager :: AddAlias(const std::string alias, const QueryID query) {
    pthread_mutex_lock(&queryManagerMutex);

    NameToID::iterator mapItr = nameToID.find(alias);
    bool retval = true;

    if( mapItr == nameToID.end() ) {
        nameToID[alias] = query;
    }
    else {
        FATAL("Cannot add alias %s to query, already a query with that name", alias.c_str());
        retval = false;
    }

    pthread_mutex_unlock(&queryManagerMutex);

    return retval;
}


QueryManager::QueryWrapper* QueryManager::getQueryInfo(string queryName) {
    QueriesNameInfoMap::iterator mapItr;
    QueryWrapper* qWrapper = NULL;

    if( nameToID.find(queryName) != nameToID.end() ) {
        qWrapper = getQueryInfo(nameToID[queryName]);
    }

    // return query information, NULL is returned in case query information is not found
    return qWrapper;
}


QueryManager::QueryWrapper* QueryManager::getQueryInfo(QueryID queryMe) {
    QueriesNameInfoMap::iterator mapItr;
    QueryWrapper* qWrapper = NULL;

    // Loop through all the queries in the system to find information about given query
    for(mapItr = mapQueriesNameInfo.begin(); mapItr != mapQueriesNameInfo.end(); mapItr++)
    {
        QueryWrapper * tempQWrapper  = (*mapItr).second;
        // Check if this information is of given query
        if(tempQWrapper->myID.IsEqual(queryMe))
        {
            // if yes, then create instance of QueryWrapper providing query's information
            qWrapper = new QueryWrapper(tempQWrapper->myID, tempQWrapper->myMap);
            break;
        }
    }

    // return query information, NULL is returned in case query information is not found
    return qWrapper;
}


bool QueryManager::RemoveQuery(string queryName) {
    //get the mutex
    pthread_mutex_lock (&queryManagerMutex);

    //get the information about the query
    QueryWrapper* qWrapper = getQueryInfo(queryName);

    int retVal = false;//initialise to case where query not found
    //if information is obtained
    if(qWrapper != NULL)
    {
        //get the bit index
        int bitIndex = qWrapper->myMap;
        //set the slot to 0 so that it can be reused for new query
        slot[bitIndex] = 0;

        idToName.erase(qWrapper->myID);

        NameToID::const_iterator iter = nameToID.begin();
        while( iter != nameToID.end() ) {
            if( iter->second == qWrapper->myID )
                iter = nameToID.erase(iter);
            else
                iter++;
        }

        //now remove the query information from table
        delete qWrapper;
        qWrapper = NULL;

        mapQueriesNameInfo.erase(queryName);

        //query removal successful
        retVal = true;
    }
    else
    {
        string warning = "Cannot remove query " + queryName + ". No such query exists!";
        WARNING("%s", warning.c_str());
    }

    //return mutex
    pthread_mutex_unlock (&queryManagerMutex);
    return retVal;
}


bool QueryManager :: GetQueryName(QueryID query, string& fillMe) {
    //get the mutex
    pthread_mutex_lock (&queryManagerMutex);

    bool retVal = false;
    IDToName::iterator itr=idToName.find(query);
    if (itr!= idToName.end()) {
        retVal=true; // found it
        fillMe=itr->second; // put the result in fillMe
    }

    pthread_mutex_unlock (&queryManagerMutex);

    return retVal;
}

bool QueryManager :: GetQueryShortName(QueryID query, string& fillMe ) {
    bool retVal = GetQueryName(query, fillMe);
    size_t len = fillMe.size() < SHORT_NAME_LENGTH ? fillMe.size() : SHORT_NAME_LENGTH;
    fillMe = fillMe.substr(0, len);

    return retVal;
}


QueryID QueryManager::GetQueryID(string queryName) {
    QueryID rez;

    //get the mutex
    pthread_mutex_lock (&queryManagerMutex);

    NameToID::const_iterator iter = nameToID.find(queryName);

    if( iter != nameToID.end() ) {
        rez = iter->second;
    }
    else {
        for( auto & el : nameToID ) {
            fprintf(stderr, "Alias: %s    Query: %llu\n", el.first.c_str(), el.second.GetInt64());
        }
        FATAL("We got asked to get id of a non-existing query %s!", queryName.c_str());
    }

    // return mutex
    pthread_mutex_unlock (&queryManagerMutex);
    return rez;
}

QueryID QueryManager::GetQueryID(const char* name) {
    string s(name);
    return GetQueryID(s);
}

int QueryManager :: QueryBitIndex(QueryID queryMe) {
    //get the mutex
    pthread_mutex_lock (&queryManagerMutex);

    QueriesNameInfoMap::iterator mapItr;
    int bitIndex = 0;

    // get query information for query having id queryMe
    QueryWrapper* qWrapper = getQueryInfo(queryMe);

    // in case qWrapper is NULL, throw error
    FATALIF(qWrapper == NULL , "Could not find the QueryID!")

        // get query bit mapping
        bitIndex = qWrapper->myMap;

    //release the mutex and return the mapping
    pthread_mutex_unlock (&queryManagerMutex);

    return bitIndex;
}

////////////////////////////////////////////////////////////////////////////////
/**
  This is weird, but this is the way to print the query.
 **/
// void Bitstring::Print () {
// 	// if this query is registered with the Query manager, we print the name
// 	// otherwise we print the bits

// 	string name;
// 	QueryManager& qm=QueryManager::GetQueryManager();
// 	if (qm.GetQueryName(*this, name)){
// 		// we found the query, print the name
// 		cerr << name << " ";
// 	} else {
// 		PrintBinary();
// 	}
// }
