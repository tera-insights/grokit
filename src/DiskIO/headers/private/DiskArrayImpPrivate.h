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

        off_t numberOfPages; // total number of pages we can use
    };

    /////////////////////
    // Stripping facility
    struct StripePair {
        unsigned uint64_t numStripe;
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

    static const __uint64_t Hash_a = 0x0000b2334cff35abUL;
    static const __uint64_t Hash_b = 0x000034faccba783fUL;
    static const __uint64_t Hash_p = 0x1fffffffffffffffUL;

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
