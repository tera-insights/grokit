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
#ifndef _EVENT_GENERATOR_H_
#define _EVENT_GENERATOR_H_

#include "DistributedCounter.h"
#include "EventGeneratorImp.h"


/**
  This is the interface for all the event generators.

  An event generator is thread that obtains event from the outer
  world (through the operating system) and transforms into a message
  to be processed by EventProcessors.

  The only functionality allowed is to create a message and to
  automatically send it to an event processor.

  For now, we do not allow swapping of these objects. Only the
  creator can kill them and kill is the only thing that can be done
  to them anyway.

NOTE: The implementatin of the methods is in EventGeneratorImp.cc
(since they are very short). Not implemeted here due to circular
dependency.
*/
class EventGenerator {
    private:
        // no copying of these objects. Force use of Swapping paradigm
        EventGenerator(EventGenerator&);
        EventGenerator& operator=(const EventGenerator&);

    protected:
        EventGeneratorImp* evGen; // the actual object performing the generation

        // keep track of how many compies of
        // this object are in the system. Only
        // the last copy is allowed to destroy
        // the object.
        DistributedCounter* numCopies;

        // should we delete the evProc in the destructor?
        bool noDelete;

    public:
        // default constructor. Does not start the generator. Must call Run()
        EventGenerator(void);
        virtual ~EventGenerator(void);

        // method to check the validity of the Event Generator
        bool IsValid(void){ return evGen!=NULL; }

        // Start the generator.
        // the generator calls in an infinite loop ProduceMessage() of EventGeneratorImp
        void Run();

        // Kill the generator; This is the only way to stop the generation
        void Kill(void);

        ////////////////////////////////
        // Swapping paradigm interface
        // swap the content with anothe EventProcessor
        void swap(EventGenerator&);

        // copy the content from another EventProcessor
        void copy(EventGenerator&);
};

#endif // _EVENT_GENERATOR_H_
