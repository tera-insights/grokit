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


#ifndef CLEANER_IMP_H
#define CLEANER_IMP_H

#include "WayPointImp.h"
#include "HashTable.h"
#include "HashTableCleanerMetaData.h"

class HashTableCleanerWayPointImp : public WayPointImp {

    private:

        // record the number of outstanding token requests
        int numWorkers;

        // this is the set of hoarded tokens we will send on to the writer waypoints
        TwoWayList <DiskWorkToken> hoardedDiskTokens;

        // this is the set of hoarded tokens to use for the CPU processing of hash table segments
        TwoWayList <CPUWorkToken> hoardedCPUTokens;

        // this asks for the disk and the CPU tokens we need to run this thing
        void SendOutRequests ();

        // this is the central hash table
        HashTable centralHashTable;

        // this sends out done messages to the writer waypoints associated with the various
        // query exits that are sent in
        void SendDone (QueryExitContainer &theseAreDone);

        bool centralHashBuilt = false;
        // function to build the central hash
        void BuildCentralHash();

    public:

        static HashTableCleanerManager metaData;

        // constructor and destructor
        HashTableCleanerWayPointImp ();
        ~HashTableCleanerWayPointImp ();

        // here we over-ride the standard WayPointImp functions
        void ProcessAckMsg (QueryExitContainer &whichOnes, HistoryList &lineage);
        void ProcessDropMsg (QueryExitContainer &whichOnes, HistoryList &lineage);
        void RequestGranted (GenericWorkToken &returnVal);
        void ProcessDirectMsg (DirectMsg &message);
        void TypeSpecificConfigure (WayPointConfigureData &configData);
        void DoneProducing (QueryExitContainer &whichOnes, HistoryList &history, int result, ExecEngineData &dataProduced);
        void ProcessHoppingUpstreamMsg (HoppingUpstreamMsg &message);
};

#endif
