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
#ifndef _QUERY_TERMINATION_TRACKER_H_
#define _QUERY_TERMINATION_TRACKER_H_

#include "TwoWayList.h"
#include "TwoWayList.cc"
#include "EfficientMap.h"
#include "EfficientMap.cc"
#include "ID.h"
#include "QueryExit.h"

/** Class that tracks the termination of queries. These objects are
  managed by ExitWayPoints but created by the FileScanner that
  provides the chunks that need to be tracked in order to detect
  query termination. The ExitWayPoint is responsible for destroying
  the objects and the FileScanner for creating them on he heap.

  For now the class just keeps track of the number of chunks seen
  and decrements a counter, provided by the File Scanner. When the
  counter is 0 the query termination is signaled. This works
  correctly if the number of chunks is known in advance and if a
  chunk cannot be dupicated. This is the case with the current
  system and it is likely to stay the same. Just to have a robust
  design, this functionality is separated into these classes.

  Now the object is small but could be large in the future. It should
  be passed by referrence.
  */

class QueryTerminationTracker {
    private:
        uint64_t counter; // the counter; when 0 the query is done

        // private constructors so we have to swap it
        QueryTerminationTracker(QueryTerminationTracker&);
        QueryTerminationTracker& operator = (QueryTerminationTracker&);

    public:
        // create a tracker set to have the given number of chunks
        QueryTerminationTracker(uint64_t noChunks);

        // swap two trackers
        void swap (QueryTerminationTracker &withMe);

        // create a tracker for zero chunks
        QueryTerminationTracker(void);

        // return true if the the query finished
        bool ProcessChunk(ChunkID &cID);

        // is the tracker valid?
        bool IsValid(void);

        // destructor
        virtual ~QueryTerminationTracker(void);
};

// Map from QueryID to QueryTerminationTrackers
typedef EfficientMap<QueryExit, QueryTerminationTracker> QueryIDQTTrackMap;

#endif // _QUERY_TERMINATION_TRACKER_H_
