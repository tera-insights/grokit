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
#ifndef _KILL_FRAMEWORK_H_
#define _KILL_FRAMEWORK_H_

#include "EventProcessor.h"
#include "EventProcessorImp.h"
#include "EventGenerator.h"
#include "EventGeneratorImp.h"
#include "Errors.h"

/**
  Event generator for the kill message sent to a communication node.
  IPC (inter-process communication) is used as follows:
  The generator blocks on a message queue until an external process inserts
  a message in the queue. At this moment, the generator sends a message to the
  corresponding event processor that it is time to exit the program.
  The program is always blocked on the event processor.
  */
class KillProcessorImp : public EventProcessorImp {
    public:
        //constructor & destructor
        KillProcessorImp()
#ifdef DEBUG_EVPROC
            : EventProcessorImp(true, "KillProcessor") // comment to remove debug
#endif
            {}
        virtual ~KillProcessorImp() {}
};

class KillProcessor : public EventProcessor {
    public:
        //empty constructor needed for initialization
        KillProcessor() {
            evProc = new KillProcessorImp();
        }
        virtual ~KillProcessor() {}
};


class KillGeneratorImp : public EventGeneratorImp {
    private:
        //message queue id
        int msgQueueId;

        //killer event processor to be stopped when the kill message is received
        KillProcessor *killer;

    public:
        //constructor & destructor
        KillGeneratorImp(KillProcessor* _killer);
        virtual ~KillGeneratorImp();

        //method invoked for each received message
        virtual int ProduceMessage(void);
};

class KillGenerator : public EventGenerator {
    public:
        //constructor that builds the implementation
        KillGenerator(KillProcessor* _killer) {
            evGen = new KillGeneratorImp(_killer);
        }
        virtual ~KillGenerator() {}
};

#endif // _KILL_FRAMEWORK_H_
