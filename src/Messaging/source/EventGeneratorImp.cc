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
#include "EventGeneratorImp.h"
#include "Errors.h"


void EventGeneratorImp::Run(void){
    if (myThread!=NULL){
        WARNING("Method Run() called more than once. Use separage Generators instead.");
        return;
    }

    myThread = new pthread_t;

    // start a thread that runs RunInternal()

    // initialize the thread
    // NULL as the second argument might need to be changed to finetune the starting
    // of the thread
    pthread_attr_t t_attr;
    int ret;

    ret = pthread_attr_init(&t_attr);
    ret = pthread_attr_setdetachstate(&t_attr, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(myThread, &t_attr, EventGeneratorImp::RunInternal, (void *)this);
    FATALIF(ret, "ERROR: return code from pthread_create() is %d.\n", ret);

    pthread_attr_destroy(&t_attr);
}

void EventGeneratorImp :: PreStart(void) {
    // nothing
}

void* EventGeneratorImp::RunInternal(void* aux){
    // convert the argument to EventProcessorImp
    EventGeneratorImp* evGenPtr=(EventGeneratorImp*) aux;

    // force the thread to exit immediately after pthread_cancel is called
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    evGenPtr->PreStart();

    while (true){ // infinite loop
        int ret = evGenPtr->ProduceMessage();
        if (ret != 0) {
            //we are told when to exit from inside ProduceMessage method
            WARNING("EventGenerator exits!");
            break;
        }
    }
}

// Kill will cancel the thread since there is no way to ensure that
// the generator will ever run ProduceMessage to completion.
void EventGeneratorImp::Kill(){
    if (myThread!=NULL){
        pthread_cancel(*myThread);
    }
}

EventGeneratorImp::~EventGeneratorImp(){
    //Kill();
    if (myThread!=NULL)
        delete myThread;
}
