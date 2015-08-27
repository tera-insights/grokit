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

#include "CPUWorkerPool.h"
#include "WorkerMessages.h"

void CPUWorkerPool :: AddWorker (CPUWorker &addMe) {
    myWorkers.Add (addMe);
}

CPUWorkerPool :: CPUWorkerPool (int numWorkers, size_t stack_size) {

    for (int i = 0; i < numWorkers; i++) {

        // create the CPU worker
        CPUWorker temp;

        // start him going
        temp.ForkAndSpin (NUMA_ALL_NODES, stack_size);

        // and add him to the pool for later use
        myWorkers.Add (temp);
    }
}

CPUWorkerPool :: ~CPUWorkerPool () {

    while (myWorkers.Length () > 0) {
        CPUWorker temp;
        myWorkers.AtomicRemove (temp);
        KillEvProc (temp);
    }
}

void CPUWorkerPool :: DoSomeWork (WayPointID &requestor, HistoryList &lineage, QueryExitContainer &dest,
        GenericWorkToken &myToken, WorkDescription &workDescription, WorkFunc &myFunc) {

    // check if the token is forged
    FATALIF(myToken.Type() != CPUWorkToken::type, "I got a fake CPU token");

    // first, go to the queue and take a worker out
    CPUWorker worker;
    if (!myWorkers.AtomicRemove (worker)) {
        FATAL ("Got into a situation where I have tried to remove a CPU worker, but none exits");
    }

    // now, assign that worker the actual work
    WorkRequestMsg_Factory (worker, requestor, myFunc, dest, lineage, myToken, workDescription);

    // done!
}

