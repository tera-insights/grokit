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
protected:
//id of the FileScanner object
TableScanID fileScannerId;

// variable that tracks how many chunks are in the file this file scaner
// works on (only comitted chunks)
// until the bulk loader is implemented, this is set to the total number of
// chunks on the disk
uint64_t currentNoChunks;

// counter for requests
uint64_t nextRequest;

//metadata file manager
FileMetadata metadataMgr;

// map to keep track of what to do when OK for requests comes back
IDToRequestMap requests;

// shortcut to the diskArray
DiskArray& diskArray;

// the Event processor that gets notified if we finish a request
EventProcessor execEngine;

// total page counter (bookkeeping)
off_t totalPages;

//////////////// Helper functions
uint64_t NewRequest(void);
