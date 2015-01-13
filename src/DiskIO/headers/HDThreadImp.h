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
#ifndef _HD_EVENT_PROC_IMP_H_
#define _HD_EVENT_PROC_IMP_H_

#include "DiskIOMessages.h"
#include "EventProcessorImp.h"
#include "EventProcessor.h"

#include <map>
#include <pthread.h>
#include <libgen.h>
#include <cstdio>
#include <cinttypes>

#define READ 1
#define WRITE 2

// forward definition on DiskArray
class DiskArray;

/** Driver for individual hard drives.

  The way the class should be used is to put a HDThread on top of
  each available disk and to access exclusively the disk through
  this object. In this way, only on way to access disks exists so
  there is no crazy contention on resources.

  Any scheduling policy with respect to ordering of the requests has
  to be implemented at higher level. The HDThreads are low leve disk
  drivers that just do what told.

  To allow signaling of job termination, the HDThreads use a
  distributed counter and send a signal when the count reaches 0.

  The HDThreads are started and coordinated by the DiskArray
  objects. Preferably, there is only one DiskArray object in the
  systems but the HDThreads do not make that assumption.

*/


class HDThreadImp : public EventProcessorImp {
    static constexpr uint64_t HD_MAGIC_NO = 0x9ddb7d9fcfc8eb78; // the first 8 bytes of "GrokIt" | md5sum
    static constexpr int32_t HD_VERSION = 0; // increase in future

    public:
        /* datastructure for the information in the first "page" of
           each stripe. The information is used to identify the stripe
           and do other things. Information can only be added to the
           struct at the end. The layout of the old version should never
           be changed.
           */
        struct Header {
            uint64_t magic; // magic number that has to match value HD_MAGIC_NO
            uint64_t arrayHash; // Has to match the Hash of the disk array
            uint64_t offset; // offset of the start of the stripe. Should be at least the size of the header
            int32_t stripeId; // ID of this stripe. First ID is 0
            int32_t version; // version of the header. If 0, just the bare header. Each version adds features
            uint64_t magicCopy; // repetition of the magic # for safety
            /* Future versions, add fields after this marker. Do not
               change the order or types of above fields */
        };

        /* function to create a stripe. If stripe cannot be created, the system fails.*/
        static void CreateStripe(char* fileName, uint64_t arrayHash, int32_t stripeId, uint64_t offset);

    private:
        int fileDescriptor;
        Header header;
        char *fileName;

        bool isReadOnly;

        // Statistics
        int frequencyUpdate; // how often to send a message up to the diskArray
        int counter; // how many pages we sent throught the life of the thread
        const double alpha; // the factor used to maintain running averages and variance
        // the update rule is exp = new*alpha+old_exp * (1-alpha);
        // the value should be between 0 and 1. Preferred value 1/frequencyUpdate

        double exp; // the expectation of the time/page in seconds
        double exp2; // the expectation of the square of the time

        DiskArray& diskArray;

        void UpdateStatistics(double time);

    public:
        HDThreadImp(const char *_fileName, uint64_t arrayHash, EventProcessor &_diskArray, int _frequencyUpdate, bool isReadOnly = false);
        virtual ~HDThreadImp();

        // which stripe is this?
        int DiskNo(void);

        /** Message handler for the MegaJobs (reading/writing pages that form a Chunk)*/
        MESSAGE_HANDLER_DECLARATION(ExecuteJob)

            friend class HDThread;
};


#endif
