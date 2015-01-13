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
#include "KillFramework.h"
#include "CommConfig.h"
#include "Errors.h"
#include "EventProcessor.h"
#include "Message.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

using namespace std;


KillGeneratorImp::KillGeneratorImp(KillProcessor* _killer) {
    killer = _killer;

    if( (msgQueueId = msgget(comm::KILL_MSG_QUEUE_ID, 0644 | IPC_CREAT)) == -1 ) {
        perror("Error getting KillGenerator messge queue");
        FATAL("Error initializing killer generator");
    }
}

KillGeneratorImp::~KillGeneratorImp() {
    killer = NULL;

    FATALIF(msgctl(msgQueueId, IPC_RMID, NULL) == -1,
        "Error destroying killer generator.");
}

int KillGeneratorImp::ProduceMessage(void) {
    msgbuf buffer;
    if (msgrcv(msgQueueId, &buffer, 1, comm::KILL_MSG_TYPE, 0) == -1) {
        WARNING("Error reading from message queue in killer generator.");
    }
    else {
        //when receiving the kill message, kill the killer event processor
        KillEvProc(*killer);

        return 1;
    }

    return 0;
}
