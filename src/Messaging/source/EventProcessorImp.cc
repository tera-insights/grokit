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
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <iostream>

#include "EventProcessorImp.h"
#include "Message.h"
#include "Errors.h"

#ifdef USE_NUMA
#include "Numa.h"
#endif

using namespace std;

EventProcessorImp::EventProcessorImp(bool _debug, const char *_dbgMsg) :
    msgQueue(_debug, _dbgMsg),  processorsMap(),
    debug(_debug), dbgMsg(_dbgMsg), forksRemaining(1), myInterface(this),
    defaultProcessor(DefaultMessageHandler),
    spin_flag(ATOMIC_FLAG_INIT)
{
    // create the mutexes and cond variables
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&died, NULL);

    dead = false;

    msgProcessor dieProc = [](EventProcessorImp& evProc, Message& msg) {
        evProc.StopSpinning();
    };

    RegisterMessageProcessor( DieMessage::type, dieProc, 0 );

    // set the spin_flag
    spin_flag.test_and_set(std::memory_order_relaxed);
}

EventProcessorImp::~EventProcessorImp() {
    if (debug) {
        cout << "EventProcessor " << dbgMsg << " is DYING." << endl;
    }

    // one last lock just in case this runs too fast
    pthread_mutex_lock(&mutex);
    for(unsigned int i = 0; i < vThreads.size(); i++) {
        //kill the threads just to be sure (there is no problem if they are already dead)
        pthread_cancel(*vThreads[i]);

        delete vThreads[i];
        vThreads[i] = NULL;
    }

    vThreads.clear();

    //set the dead variable to signal we are dead
    dead = true;

    //signal all the waiting threads that we are dead
    pthread_cond_broadcast(&died);

    pthread_mutex_unlock(&mutex);

    // distroy the mutex
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&died);

    if (debug) {
        cout << "EventProcessor " << dbgMsg << " is DEAD." << endl;
    }
}

void EventProcessorImp::RegisterMessageProcessor(off_t Type, msgProcessor Proc, int Priority) {
    // put the method in
    processorsMap[Type] = Proc;
    msgQueue.AddMessageType(Type, Priority);
}

void EventProcessorImp::RegisterDefaultMessageProcessor( msgProcessor Proc ) {
    defaultProcessor = Proc;
}

void EventProcessorImp::PreStart(void) {
    // Do nothing
}

void EventProcessorImp::Spin() {
    // ensure we die instanly when somebody calls cancel
    pthread_setcanceltype(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcancelstate(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    PreStart();

    // repeat forever
    while (spin_flag.test_and_set(std::memory_order_relaxed)) {
        // get new message from the queue (blocks until a message is available)
        Message& cMessage = msgQueue.RemoveMessage();

        ProcessorMap::iterator cur = processorsMap.find(cMessage.Type());
        // we cannot possibly get a message that we do not suppoprt

        if( cur != processorsMap.end() ) {
            // get the correct processor
            msgProcessor proc = cur->second;
            // call the processor
            (proc)(*this, cMessage);
        }
        else {
            // Don't have a processor registered for this message type, use the
            // default handler.
            (defaultProcessor)(*this, cMessage);
        }

        delete (&cMessage);
    }
}

void EventProcessorImp::StopSpinning(void) {
    spin_flag.clear(std::memory_order_relaxed);
}

void EventProcessorImp::ProcessMessage(Message& msg) {
    if (!dead)
        msgQueue.InsertMessage(msg);
}

void EventProcessorImp::WaitForProcessorDeath(void) {
    pthread_mutex_lock(&mutex);
    while (!dead) {
        pthread_cond_wait(&died, &mutex);
    }
    pthread_mutex_unlock(&mutex);
}

void EventProcessorImp::Seppuku() {
    if (debug) {
        cout << "EventProcessor " << dbgMsg << " is SEPPUKU." << endl;
    }

    pthread_mutex_lock(&mutex);
    for(unsigned int i = 0; i < vThreads.size(); i++) {
        //kill the threads
        pthread_cancel(*vThreads[i]);
    }

    //set the dead variable to signal we are dead
    dead = true;

    //signal all the waiting threads that we are dead
    pthread_cond_broadcast(&died);

    pthread_mutex_unlock(&mutex);
}


void* EventProcessorImp::ForkAndSpinThread(void* aux) {
    // convert the argument to EventProcessorImp
    EventProcessorImp* evProcPtr=(EventProcessorImp*) aux;

    // call Spin on the event processor
    evProcPtr->Spin();

    //pthread_exit(NULL);
    return NULL;
}

bool EventProcessorImp::ForkAndSpin(int node, size_t stack_size) {
    //are we dead? can we even start a thread?
    pthread_mutex_lock(&mutex);
    if ((dead == true) || (forksRemaining == 0)) {
        pthread_mutex_unlock(&mutex);
        return false;
    }

    // yes we can so decrement the number of threads allowed in the future
    forksRemaining--;

    // create the thread datastructure then call the thread creation.
    pthread_t *threadPtr = new pthread_t;

    // bookkeeping for threads started
    vThreads.push_back(threadPtr);

    // NULL as the second argument might need to be changed to finetune the starting
    // of the thread
    pthread_attr_t t_attr;
    int ret;

    ret = pthread_attr_init(&t_attr);
    ret = pthread_attr_setstacksize(&t_attr, stack_size);
    ret = pthread_attr_setdetachstate(&t_attr, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(threadPtr, &t_attr, EventProcessorImp::ForkAndSpinThread, (void *)this);
    FATALIF(ret, "ERROR: return code from pthread_create() is %d.\n", ret);

    pthread_attr_destroy(&t_attr);

    pthread_mutex_unlock(&mutex);

#ifdef USE_NUMA
    // Do we need to pin the thread to a NUMA node?
    if (node != NUMA_ALL_NODES) {
        // wrap the number of nodes arround
        node = node % numaNodeCount();

        // first figure out the cpu's that correspond to this node
        cpu_set_t cpus[MAX_CPU_SETS]; // this allows 640 cpus
        numa_nodes_to_cpus(node, cpus, MAX_CPU_SETS);

        // pin the thread down
        pthread_setaffinity_np(threadPtr, MAX_CPU_SETS, cpus);
    }
#endif // USE_NUMA

    return true;
}

void EventProcessorImp::DefaultMessageHandler( EventProcessorImp& evProc, Message& msg ) {
    std::string errMsg = "EventProcessor got unexpected message of type: ";
    errMsg += msg.TypeName();
    FATAL(errMsg.c_str());
}

EventProcessor EventProcessorImp :: Self(void) {
    EventProcessor ret(this);
    return ret;
}
