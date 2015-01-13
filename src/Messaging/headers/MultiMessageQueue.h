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

#ifndef _MULTIQUEUE_H_
#define _MULTIQUEUE_H_

#include <pthread.h>
#include <list>
#include <map>
#include <functional>

// maximum number of types that can be handled by a queue
#define MAX_NUM_TYPES 256


// forward definition of message and EventPrcessor
class Message;
class EventProcessorImp;

/**
  Class to implement message passing between threads that suppoprts
  multiple message types simultaneously.

  The class is intended to have a single reader that wayts for multiple
  types of messaes simultaneously and multiple readers.

  The class has to be thread safe. There is no limit to the size of the queue
  implemented.

  The class is designed to manipulate only messages, not data. The
  only assumption the class makes is that only one thread works on a
  message at a time (assumption about usage of messages).

  The messages are created by the writer and distroied by the
  reader. Only refferences are passed around.

  MultiMessageQueue does not look at the messages, just keeps track of their
  type.
  */

class MultiMessageQueue {
    public:
        // struct to store information about the available messages.
        // The struct is used to pass informatin to the user defined
        // message selection function
        struct TypeInfo {
            off_t type; // the type as returned by Message::Type()
            int priority; // as specified at type registration
            int age; // the age of the oldest message of this type
            int numMessages; // the number of messages of this type
        };

        // type for functions to decide what is the next message to be processed
        //typedef off_t (*DecisionFunction)(EventProcessorImp& , TypeInfo*, int);
        typedef std::function<off_t(EventProcessorImp&, TypeInfo*, int)> DecisionFunction;

#include "MultiMessageQueuePrivate.h"

    public:
        MultiMessageQueue(bool _debug = false, const char *_dbgMsg = NULL);
        virtual ~MultiMessageQueue();

        // if the type is already in, the priority is changed
        // the priority of the type is used to compute the priority of a message
        void AddMessageType(off_t Type, int Priority);

        // Add a message to the queue
        // if the type is not supported, the message is not added (i.e. it is discarded)
        // this never blocks more than the time it takes to que the message
        void InsertMessage(Message& Payload);

        // this method results in blocking if no message is stored
        // the internal payload is placed in Payload
        // the messages of the same type are returned in order
        // messages of different types are retreived in the increasing priority
        // order (default behavior) or the order controlled by the user specified function
        Message& RemoveMessage();

        // Function to register another decision policy for what message should be
        // extracted from the message queue
        // the function looks like:
        //    int DecFct(EventProcessor& _obj, TypeInfo* arrayTypeInfo, int num)
        // the _obj will be the parameter passed at registration
        // arrayTypeInfo is an array containint TypeInfo structures about all the
        // messages available and num is the number of of TypeInfo elements
        // only types for which there is at least one message available are placed in the array
        // The registered function MUST return one of the types provided
        // The function is called only if messages are available and just before
        // RemoveMessage returns. That means it is called in the context of the thread
        // that calls RemoveMessage
        // If only a thread is used to pull out messages for an EventProcessor
        // there is no need for synchronization even if the function acesses/modifies state
        void RegisterDecisionPolicy(EventProcessorImp& _obj, DecisionFunction fct);

        // return the number of pending messages
        int GetSize();
};

#endif // _MULTIQUEUE_H_
