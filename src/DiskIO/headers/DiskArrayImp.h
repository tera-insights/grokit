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
#ifndef _DISKARRAY_IMP_H_
#define _DISKARRAY_IMP_H_

#include "EventProcessorImp.h"
#include "HDThread.h"
#include "Machine.h"
#include "DiskMemoryAllocator.h"


#include <vector>
#include <sqlite3.h>

/** Disk array driver. Ideally, there is only one in the system and
  all requests go through it. We ensure this is the case by
  providing the static function GetDiskArray() that gives a handler
  to the only DiskArray in the system.

  The uniqueness of the instance is enforced in the interface class
  not here. This gives a little more flexibility in case we want to
  implement multiple disk arrays in the future.

  The disk array uses it's own metadata configuration file. It
  defines the struct DisksMetadata that contains all the info it
  needs. Once created, this metadata will not change.q

  For now, there is no deletion.

    Tasks:

    A. Get high level requests for pages in the "flat page space" and
    translate them into striped requests that get sent to the HDTHreads

    C. Implement the striping strategy (for now, just the simple random striping)

    D. Manage the space in the repository. A very simple approach is
    used now that consists in maintaining a threshold beyound which
    all pages are empty.

    Requirements:

    A. Each request must specify and EventProcessor that gets notified
    when the request completes. The DiskArray will create a
    DistributedCounter for each request that is decremented by each
    HDThread when peforming the task. The HDThread that reduces the
    counter to 0 notifies the caller.

    ToDo:
    1. Allow redundancy in the HDThreads to mask disk problems
    a. add messages from HDThreads to the DiskArray indicating disk problems

    2. Use statistics in a creative way

    Future Improvements:
    0. allow deletions and manage space better. Bitmap mask for available pages? Vector of free ranges?
    The vector of free ranges is probably good enough since all relations are large
    1. allow the addition and deletion of disks on the fly ?!?
    2. implement replication to survive disk failures

    (if replication is implemented, taking disks offline is probably doable).

*/

class DiskArrayImp : public EventProcessorImp {
    public:
        struct DiskStatisticsData {
            double exp; // exp of seconds/page
            double var; // variance of seconds/page
        };

    private:
        struct DisksMetadata{
            // Function parameter #1
            unsigned long stripeParam1;

            // Function parameter #2
            unsigned long stripeParam2;

            //the number of hard-disks used for striping the file
            unsigned long HDNo;

            off_t pageMultExp; // the number of MMap pages that make a disk page (as an exponent)
            // this is used to allign requests for space on the disk on larger pages

            unsigned long arrayID; // the id of the array in the metadata

	  uint64_t arrayHash; // the hash of the array. This should be unique

            off_t numberOfPages; // total number of pages we can use
        };

        /////////////////////
        // Stripping facility
        struct StripePair {
            unsigned int numStripe;
            off_t numPage;
        };

        // this is the hash that maps the flat page space into the striped space
        // this is the single place where the function exists
        // the parameters for the function come from the file metadata
        StripePair StripingHash(off_t numPage);

        //////////////////////
        // state
        DisksMetadata meta; // the metadata from file
        EventProcessor* hds; // vector of hard drives (HD Threads)
        bool modified; // set true if we need to write the metadata

        pthread_mutex_t lock; // guard for AllocatePages.
        // we use a spinlock instead of a mutex since the function AllocatePages
        // should work very fast so there is no point in taking the processor away
        // from the calling thread.


        //////////// disks statistics
        std::vector<DiskStatisticsData> stats; // per disk statistics

        double expectation; // the overall average, allows computation of throughput
        // as HDNo*expectation
        double sampleVariance; // the variance accross the disks from expectations
        double averageVariance; // the average variance of disks

        off_t totalPages; // total number of pages read by the system
        off_t pagesAtLastCall;

        // allign the page to the larger disk page using pageMultiplier
        off_t PageAllign(off_t page);

        static constexpr __uint64_t Hash_a = 0x0000b2334cff35abUL;
        static constexpr __uint64_t Hash_b = 0x000034faccba783fUL;
        static constexpr __uint64_t Hash_p = 0x1fffffffffffffffUL;

        inline __uint64_t PageHash(const __uint64_t x, const __uint64_t b = Hash_b){
            __uint128_t y = (__uint128_t) x * Hash_a + b;
            __uint64_t yLo = y & Hash_p; // the lower 61 bits
            __uint64_t yHi= (y>>61) ; // no need to mask to get next 61 bits due to low constant H_a
            __uint64_t rez=yLo+yHi;

            // Below statement essentially means: __uint64_t rez2 = (rez<H_p ? rez : rez-H_p);
            // But only problem below is, we do not get zero result. May be we could say, it's
            // not the problem, but the solution. This nifty trick solves the branching statements,
            // which may or may not shown to be problematic
            __uint64_t rez2 = (rez & Hash_p) + (rez >> 61);
            return rez2;
        }

        // space manager
        DiskMemoryAllocator diskSpaceMng;

    public:
        // constructor & destructor
        DiskArrayImp(bool isReadOnly);
        virtual ~DiskArrayImp();

        // method to allocate pages. Behaves atomically. Should be called to
        // determine where to write info.
        off_t AllocatePages(off_t _noPages, int relID);

        // method to delete all content of a relation
        void DeleteRelationSpace(int relID);

        // statistics
        void PrintStatistics(void);

        // update metadata on disk
        void Flush(void);

        // update within another action (so that we can run a single transactin)
        void Flush(sqlite3* db);

        // get the ID
        int getArrayID(void){ return meta.arrayID; }

        // ask about the amount of IO
        off_t NumPagesProcessed(void);
        off_t NumPagesDelta(void); // since last call

        // this message is received when the top wants an operation performed
        MESSAGE_HANDLER_DECLARATION(DoDiskOperation);

        // this mesage is sent by the HDThreads with statistics on throughput
        MESSAGE_HANDLER_DECLARATION(ProcessDiskStatistics);
};

//////////////////
// Inlined methods

inline 	off_t DiskArrayImp::NumPagesProcessed(void){
    return totalPages;
}

inline 	off_t DiskArrayImp::NumPagesDelta(void){
    int rez=totalPages-pagesAtLastCall;
    pagesAtLastCall=totalPages;
    return rez;
}


inline void DiskArrayImp::Flush(sqlite3* db){ diskSpaceMng.Flush(db); }

inline void DiskArrayImp::DeleteRelationSpace(int relID){
    diskSpaceMng.DiskFree(relID);
}

inline off_t DiskArrayImp::PageAllign (off_t _noPages){
    return ( _noPages + ((1<<meta.pageMultExp)-1)) & ~((1<<meta.pageMultExp)-1);
}

inline off_t DiskArrayImp::AllocatePages(off_t _noPages, int relID){

    // first allign the page request
    _noPages =  PageAllign(_noPages);

    return diskSpaceMng.DiskAlloc(_noPages, relID);
}


#endif // _DISKARRAY_IMP_H_
