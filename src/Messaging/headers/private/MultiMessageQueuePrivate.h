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


private:
    // Internal base class for all the message types
    class InternalMessage {
        private:
            // for type, ask the message
            // priority determined by the queue it is sitting in

            // this is the message transmited. The payload should be casted
            // to the correct type based on type field
            Message* payload;

            int timestamp; // the time when the message got generated

        public:
            // constructor
            InternalMessage(Message& Payload, int _timestamp) :
                payload(&Payload), timestamp(_timestamp) {
                }

            // default constructor
            InternalMessage(void);

            // copy constructor
            InternalMessage(const InternalMessage&);
            InternalMessage& operator=(const InternalMessage& aux);

            off_t GetType(void);
            inline int GetTimestamp() { return timestamp; }
            inline Message& GetPayload(void) { return *payload; }
    };

    // bookkeeping for each type used for keeping track of message queues and info about types
    class TypeBookkeeping {
        private:
            std::list<InternalMessage> queue;
            int priority; // the priority of this type
            off_t type; // the type of the message stored here

        public:
            TypeBookkeeping(int _priority, off_t _type):
                priority(_priority), type(_type){ }

            void InsertMessage(InternalMessage& msg){ queue.push_back(msg); }

            // return the next message in the queue
            // it is an error to ask for the next message if none is there
            InternalMessage NextMessage(void);

            // return false if no message in the queue of this type
            bool GetTypeInfo(int curr_timestamp, TypeInfo* where);
            int GetSize(void){ return queue.size(); }
    };

    // debugging  facility
    bool debug;
    const char *dbgMsg;

    // the user provided decision function
    // if NULL, we call our own
    DecisionFunction decFct;

    // EventProcessor that implements the decision function
    EventProcessorImp& decProcessor;

    // the default Decision Function
    static off_t defaultDecisionFunction(EventProcessorImp& _obj, TypeInfo* arrayTypeInfo, int num);

    // we keep messages grouped on types
    // we use a map based on type to indicate the queue where the types are
    // the map stores the priority of each type as well
    std::map<off_t,TypeBookkeeping> typeMap;

    // List to keep all unregistered messages.
    std::list<InternalMessage> unregisteredMessages;

    int timestamp; // the current timestamp
    int numMessages; // total number of messages in (for efficiency)

    pthread_mutex_t mutex; // mutex to ensure exclusive manipulation of the queue
    pthread_cond_t condVar; // condition variable for the reader
