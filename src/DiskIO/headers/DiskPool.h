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
#ifndef _DISK_POOL_H_
#define _DISK_POOL_H_

#include "EfficientMap.h"
#include "ID.h"
#include "History.h"
#include "Tokens.h"
#include "Chunk.h"

#include <map>
#include <string>
#include <vector>
#include <utility>
#include <cstddef>

/** Class to provide an interface from the disk to the execution engine.

  This class is responsible for translating the requests form the
  execution engine format to the disk format. It also keeps track of tokens.

  The class has a signleton implementation. The entire system can
  use this class to send disk requests.

WARNING: for now the code is not multithreading safe since this
class should be used only from whithing the uni-threaded execution
engine.

*/

class DiskPool {
    public:
        typedef std::pair<int64_t, int64_t> ClusterRange;
        typedef std::vector<ClusterRange> ClusterRangeList;
    
    private:
        typedef EfficientMap< TableScanID, EventProcessor > EVProcMap;
        // map from TableScanID to event processor to keep track of scanners and writers
        EVProcMap files;

        typedef std::map< TableScanID, off_t > SizeMap;
        // map from ID to number of chunks
        SizeMap sizes;
        
        typedef std::map< TableScanID, ClusterRangeList > ClusterRangeMap;
        ClusterRangeMap clusterRanges;

    public:
        // start the disk pool
        DiskPool():
          files(),
          sizes(),
          clusterRanges()
        {}

        // destructor
        ~DiskPool(){Stop();}

        /** Stop kills all the ChunkReaderWriters for all known relations */

        void Stop();


        /* The only way fo anybody in the execution engine to get something read/written to the disk.

           The requestor, lineage are used to route replyes back. dest
           specifies what queriy exits will be attached with the reply

           Token certifies access to the resource.

           if useUncompressed is set, uncompressed data is read, otherwise
           compressed. This is just a hint and can be changed by the disk
           part.

           colsToProcess is a set of pairs that specify the logical--physcal columns.

*/

        void ReadRequest(ChunkID& id, WayPointID &requestor, bool useUncompressed,
                HistoryList &lineage, QueryExitContainer &dest,
                GenericWorkToken& token, SlotPairContainer& colsToProcess);

        void WriteRequest(ChunkID& id, WayPointID &requestor, Chunk& chunk,
                HistoryList &lineage, QueryExitContainer &dest,
                GenericWorkToken& token, SlotPairContainer& colsToProcess);

        void UpdateClusterRange(ChunkID& id, WayPointID &requestor,
                ClusterRange& range);

        void Flush (TableScanID id);

        /* If the file scanner associated with the name is not started, it
           will be started by this function. If already running, nothing happens.

           In either case, the TableScanID that is used to form all ChunkID
           comming from this file scanner is returned.

           Whenever a new entity in the execution engine needs to talk to a
           File, it has to use this interface.

           numCols indicates how many columns we have (or should have)
           */
        TableScanID AddFile(std::string name, int numCols);
        // function to stop the file. After this the scanner is not available and
        // future attempts to talk to it will result in errors
        // this does not block
        void StopFile(TableScanID id);
        // function to get the number of chunks a file has
        off_t NumChunks(TableScanID id);

        ClusterRangeList ClusterRanges(TableScanID);

        void DeleteContent(std::string);

        void DeleteRelation(std::string name);

};

extern DiskPool globalDiskPool; // global object that manages the disk  pool


#endif // _DISK_POOL_H_
