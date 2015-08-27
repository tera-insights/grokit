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
#ifndef _CHUNK_READER_WRITER_IMP_H_
#define _CHUNK_READER_WRITER_IMP_H_

#include <vector>
#include <map>
#include <utility>
#include <cstddef>
#include <pthread.h>

#include "DiskIOMessages.h"
#include "DiskArray.h"
#include "DiskIOData.h"
#include "FileMetadata.h"

/** Class to implement the mid-level File access.
  Its main job is to coordinate the reading and the writing of chunks

  The actual I/O is performed through a DiskArray (the global disk
  array for now).

  The class needs to both read and write chunks since the hard drive
  drivers can only talk to a single higher task. Also this solves a
  lot of headakes with making sure that the higher tasks know about
  writes as well as reads.

  The class is designed to do asynchronous reads and writes. Nothing
  blocks while the requests are pending and the higher file scanner
  code gets notified when a job is done.

  Since some tasks might require very few pages, the order of
  finishing the jobs might be different than the order of the
  requests. It is the job of the higher part to reorder the
  accnoledgements if it so desrires (this part acts like the IP
  layer in TCP/IP).

  On Reading, the class creates chunks so it does all the heavy
  lifting in terms of memory allocation and initialization. This is
  why this class should be asynchronous and allow multi-threading.
  */

class ChunkReaderWriterImp : public EventProcessorImp {

#include "ChunkReaderWriterImpPrivate.h"

    public:
        typedef std::pair<int64_t, int64_t> ClusterRange;
        typedef std::vector<ClusterRange> ClusterRangeList;

        // the file scanner will get the messages when the job is done
        ChunkReaderWriterImp(const char* _scannerName, uint64_t _numCols, EventProcessor& _execEngine);
        virtual ~ChunkReaderWriterImp();

        // method to get the number of chunks
        // it is safe to call this only after constructor but before
        // forkAndSpin
        off_t GetNumChunks(void){ return metadataMgr.getNumChunks(); }

        ClusterRangeList GetClusterRanges(void) {
          off_t nChunks = metadataMgr.getNumChunks();
          ClusterRangeList ret;

          for( off_t i = 0; i < nChunks; i++ ) {
            ret.push_back(metadataMgr.getClusterRange(i));
          }

          return ret;
        }

        //////////////////////////
        // MESSAGE HANDLERS

        // this message is received when the HD threads finish a chunk
        MESSAGE_HANDLER_DECLARATION(ChunkRWJobDone);

        // this message is received when the upper part needs a chunk read
        MESSAGE_HANDLER_DECLARATION(ReadChunk);

        // this message is received when the upper part whants the columns in a chunk writen
        // this is a low level interface; a higher level interface might be supported in the future
        MESSAGE_HANDLER_DECLARATION(WriteChunk);

        MESSAGE_HANDLER_DECLARATION(FlushFunc);

        MESSAGE_HANDLER_DECLARATION(DeleteContentFunc);

        MESSAGE_HANDLER_DECLARATION(ClusterUpdateFunc);
};


#endif // _CHUNK_READER_WRITER_IMP_H_
