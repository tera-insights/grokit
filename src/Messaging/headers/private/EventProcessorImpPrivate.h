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
    // the message queue (owned by the EventProcessor)
    MultiMessageQueue msgQueue;

    // mapping of message types to processing functions
    typedef std::map< off_t, msgProcessor > ProcessorMap;
    ProcessorMap processorsMap;

    // Default message processor
    msgProcessor defaultProcessor;

    // keep track of thread structures started to stop threads in destructor
    std::vector< pthread_t* > vThreads;

    // general purpose mutex to be used in this class. More refined mutexes can be defined
    // and used in derived classes
    pthread_mutex_t mutex;

    ///////////
    // Facilities to wait for the event processor to die
    // are we dead (no wait on conditional variables)
    volatile bool dead;
    // conditional variable that signals the death of the event processor
    pthread_cond_t died;

    /////////////
    // Helper functions
    // helper function to start a thread running Spin()
    // the argument passed is the pointer to the EventProcessor on which Spin is executed
    static void* ForkAndSpinThread(void*);

    // protect the copy constructor and the asignment operators to ensure
    // they are never used
    EventProcessorImp(const EventProcessorImp&);
    EventProcessorImp& operator= (const EventProcessorImp&);
