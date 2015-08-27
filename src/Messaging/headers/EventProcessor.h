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
// for Emacs -*- c++ -*-

#ifndef _EVENTPROCESSOR_H_
#define _EVENTPROCESSOR_H_

#include "DistributedCounter.h"
#include "Numa.h"
#include "TwoWayList.h"
#include "TwoWayList.cc"

#include <cstddef>

/**
  if Event processing debuging is needed uncomment the followint
  */

// forward definitions
// implementation of the event processor
class EventProcessorImp;
// forward definition of all messages
class Message;


/**
  Base class to implement all the front classes of event processors.
  These are the only classes that can construct the real EventProcessors.

  When a new EventProcessor is designed, a class should be derived
  from this base class and the constructor should just build the object.

WARNING1: derived interface classes should not redefine any of the
methods (including the destructor) except for defining a
constructor with the same arguments as the real EventProcessor.

In order to allow for simple manipulation of objects
*/
class EventProcessor {
    private:
        static const constexpr size_t DEFAULT_STACK_SIZE = 8L * 1024L * 1024L;

    public:
        // Copying is allowed, this is just a shallow front-end class
        EventProcessor(const EventProcessor&) noexcept;
        EventProcessor& operator=(const EventProcessor&);

        // Moving is also allowed
        EventProcessor(EventProcessor &&) noexcept;

    private:
        // Clears out this event processor, either when the destructor is called or the
        // event processor is being reassigned.
        void Clear(void);

    protected:
        // this is the real event Processor
        EventProcessorImp* evProc;

        // keep track of how many compies of
        // this object are in the system. Only
        // the last copy is allowed to destroy
        // the object.
        DistributedCounter* numCopies;

        // should we delete the evProc in the destructor?
        bool noDelete;

        // special constructor for EventProcessorImp to create a interface object for itself
        EventProcessor(EventProcessorImp* const);

    public:
        // constructor. We do not mimic the constructor of EventProcessorImp
        // since that class never gets directly instantiated
        // we provide this constrctor so that EventProcessors that
        // do nothing can be build so that they can be placed in containers and Swapped into
        EventProcessor(void) noexcept;

        // method to check the validity of the Event Processor
        bool IsValid(void) const { return evProc!=nullptr; }

        // This method is called in the thread running the writer
        // All the other methods in EventProcessor run in a separate thread
        // The message results in putting the message in the message queue
        // Thread safety is ensured by the queue so there is no problem with simultaneous execution
        virtual void ProcessMessage(Message& msg);


        // this method creates another thread running the main event processing loop
        // if the return false, than no thread was started
        // since the maximum number of threads allowed by this event is already reached
        // If node is provided, it pins down the processor to the specified
        // node To avoid overly complicated scenarios, the node number is
        // wrapped around That means that if there are only 4 nodes,
        // specifying value 7 is the same as 3 (7 % 4).
        // Since the application
        // WARNING: this method only starts one thread, not all the allowed threads
        // it has to be executed repeteadly to start multiple threads
        bool ForkAndSpin(int node=NUMA_ALL_NODES, size_t stack_size=DEFAULT_STACK_SIZE);

        // kill the event processor (the threads running inside it)
        void Seppuku(void);

        // When this functin is called, the thread calling it blocks until the event
        // processor gets the Die message. This is useful to write main programs that
        // wait for the whole thing to finish
        void WaitForProcessorDeath(void);

        ////////////////////////////////
        // Swapping paradigm interface
        // swap the content with anothe EventProcessor
        void swap(EventProcessor&);

        // copy the content from another EventProcessor
        void copy(const EventProcessor&);


        // destructor
        virtual ~EventProcessor();

        // we need this to give EventProcessorImp access to
        // the constructor EventProcessor(EventProcessorImp*)
        friend class EventProcessorImp;
};

// STL compatible swap
inline
void swap( EventProcessor& a, EventProcessor&b ) {
    a.swap(b);
}

// container to keep event processors
typedef TwoWayList<EventProcessor> EventProcessorContainer;


// Kill an EventProcessor and wait for it to die.
// This is the preffered method (as oppposed to DieMessage_Factory) since
// it ensures that the processor actually died.
inline void KillEvProc(EventProcessor& evProc) {
    evProc.Seppuku();
}

#endif // _EVENTPROCESSOR_H_
