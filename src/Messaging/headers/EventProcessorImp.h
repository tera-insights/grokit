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

#ifndef _EVENTPROCESSORIMP_H_
#define _EVENTPROCESSORIMP_H_

#include <map>
#include <vector>
#include <pthread.h>
#include <functional>
#include <atomic>

#include "EventProcessor.h"
#include "MultiMessageQueue.h"
#include "MessageMacros.h"


// forward definition of all messages
class Message;

/**
  Base class to implement all the event processors.

  The class uses a MultiMessageQueue to get messages, and calls functions that
  can process the messages. The implementation is generic so extra functionality
  can be added for actual event processors.

  The EventProcessor owns the MultiMessageQueue and just makes it available
  to writtes so messages can be sent.

  The class keeps a mapping of integer types to methods that can process that
  message type.

  To add capabilities to process a new message type, a method has to be written
  that converts the basic message into the correct message type.

    WARNING1: While the datastructures to manage the event processor
    data are protected by a mutex, the activity of the message
    processing functions is not protected at all. For this reason, by default,
    only one thread can be started to process the messages (forksRemaining=1).
    In the derived class, if forksRemaining is increased, mutexes inside
    the message processing functions should be used as needed. The existing mutex,
    call mutex can be used or other, more refined mutexes should be defined.

    WARNING2: The event processors should be created on the heap and
    destroied explicitly. The call to ForkAndSpin() is nonblocking
    and will return almost immediately.

    Instantiating this class directly does not make much sense. It
    should be derived from.
*/
class EventProcessorImp {
    public:
        // type of methods that can process messages
        typedef std::function<void(EventProcessorImp&, Message&)> msgProcessor;

#include "EventProcessorImpPrivate.h"

    protected:
        // data for debuging purposes
        bool debug;
        const char *dbgMsg;

        // Flag to check while spinning
        std::atomic_flag spin_flag;

        // some event processors need to be cloned so that multiple threads work in parallel
        // this has to be done carefully so by default no event processor is forkable
        // set to 1 so only one thread runs
        int forksRemaining;

        // interface object that can be used to communicate with this event processor
        EventProcessor myInterface;

        // register a processor for a new type of message
        // the Type is the numeric type associated with the type of message
        // proc is the method dealing with this type of message
        // Priority is the priority of this type (default highest)
        // The constructor of any derived class that has clear functionality
        // has to call this method for all the messages tha need to be processed
        // Only designers of Event Processors should call this otherwise it can be quite dangerous
        // If the method is called multiple times for the same type, the old info is overwritten
        // this allows "overloading" of the Die behavior
        void RegisterMessageProcessor(off_t Type, msgProcessor Proc, int Priority=1);

        // Change the default message processor to the function given by Proc
        // This function will be called if no other message handler is registered for
        // a message type.
        // If not overridden, the default message handler will simply fail.
        void RegisterDefaultMessageProcessor( msgProcessor Proc );

        // set the maximum number of threads that an EventProessor supports
        // this does not start that many threads, it just sets the limit
        void SetMaxThreads(int max) {
            pthread_mutex_lock(&mutex);
            forksRemaining = max;
            pthread_mutex_unlock(&mutex);
        }

        // Called before starting to spin. Allows for initialization.
        // Default implementation does nothing.
        virtual void PreStart(void);

        // Start the event processing loop. This goes forever but does not do
        // busy wayting
        // everybody should use ForkAndSpin instead of Spin to start a thread running the Spin()
        virtual void Spin(void);

        // Makes the event processor stop spinning after the current message.
        void StopSpinning(void);

        // Method for an event processor to get an interface object for itself.
        EventProcessor Self(void);

    public:
        // constructor
        // if debug is true, debugging messages are printed (contain string dbgMsg)
        EventProcessorImp(bool _debug = false, const char *_dbgMsg = NULL);

        // This method is called in the thread running the writer
        // All the other methods in EventProcessor run in a separate thread
        // The message results in putting the message in the message queue
        // Thread safety is ensured by the queue so there is no problem with simultaneous execution
        void ProcessMessage(Message& msg);


        // this method creates another thread running the main event processing loop
        // if the return false, than no thread was started
        // since the maximum number of threads allowed by this event is already reached
        // WARNING: this method only starts one thread, not all the allowed threads
        // it has to be executed repeteadly to start multiple threads
        // the default is defined
        bool ForkAndSpin(int node = NUMA_ALL_NODES, size_t stack_size = EventProcessor::DEFAULT_STACK_SIZE);

        // When this functin is called, the thread calling it blocks until the event
        // processor gets the Die message. This is useful to write main programs that
        // wait for the whole thing to finish
        void WaitForProcessorDeath(void);

        // method to commit suicide. This should be called if the event processor wants to kill itself
        void Seppuku();

        // destructor
        virtual ~EventProcessorImp();

        // Declare default message handler
        static void DefaultMessageHandler( EventProcessorImp& evProc, Message& msg );
};


#endif // _EVENTPROCESSORIMP_H_
