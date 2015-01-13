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
#ifndef _EVENT_GENERATOR_IMP_H_
#define _EVENT_GENERATOR_IMP_H_

#include <stdlib.h>
#include <pthread.h>


/**
  Base class for implementation of all Event Generators.
  */
class EventGeneratorImp {
    protected:
        pthread_t* myThread;

        // this is what the pthread runs
        static void* RunInternal(void*);

    public:
        EventGeneratorImp(): myThread(NULL){}

        // infinite loop to call ProduceMessage
        void Run();

        // This method is called once, right before the first ProduceMessage call.
        // The default implementation does nothing.
        virtual void PreStart(void);

        // this method needs to be defined in derived classes
        // this method implements the basic functionality only , not the infinite looop
        // usually the implementation would block on some external event and translate it
        // into an EventProcessor message
        // it is the business of the particular EventGenerator to know where to sent
        // the message and what messagee to send
        virtual int ProduceMessage()=0;

        // this method kills the event generator. This is the only way to kill it
        void Kill();

        virtual ~EventGeneratorImp();
};

#endif // _EVENT_GENERATOR_IMP_H_
