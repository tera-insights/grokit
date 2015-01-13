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
#include <assert.h>
#include <stdio.h>

#include "Errors.h"
#include "Message.h"
#include "EventProcessor.h"
#include "MultiMessageQueue.h"
#include "EventProcessorImp.h"

using namespace std;

// needed to initialize references in MultiMessageQueue
EventProcessorImp DummyEventProcessorImp;

/////////////////////////
// Internal Message stuff

MultiMessageQueue::InternalMessage::InternalMessage(const InternalMessage& aux):
	payload(aux.payload), timestamp(aux.timestamp){}

MultiMessageQueue::InternalMessage::InternalMessage(void):
	payload(NULL), timestamp(0){}

MultiMessageQueue::InternalMessage&
MultiMessageQueue::InternalMessage::operator=(const InternalMessage& aux) {
	payload = aux.payload;
	timestamp = aux.timestamp;

	return *this;
}

off_t MultiMessageQueue::InternalMessage::GetType(void){
	return payload->Type();
}


/////////////////////////
// TypeBookkeeping stuff

MultiMessageQueue::InternalMessage
MultiMessageQueue::TypeBookkeeping::NextMessage(void){
	assert(!queue.empty());
	InternalMessage ret=queue.front();
	queue.pop_front();
	return ret;
}

bool MultiMessageQueue::TypeBookkeeping::GetTypeInfo(int curr_timestamp, TypeInfo* where){
	if (queue.empty()){
		return false;
	}

	InternalMessage msg=queue.front();
	where->type=type;
	where->priority=priority;
	where->age=curr_timestamp-msg.GetTimestamp();
	where->numMessages=queue.size();

	return true;
}


////////////////////////
// MultiMessageQueue stuff

MultiMessageQueue::MultiMessageQueue(bool _debug, const char *_dbgMsg) :
	debug(_debug), dbgMsg(_dbgMsg), decProcessor(DummyEventProcessorImp) {
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&condVar, NULL);
	timestamp=0;
	decFct = defaultDecisionFunction;
	numMessages = 0;
}

MultiMessageQueue::~MultiMessageQueue() {
	// deleate messages in the queue
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&condVar);
}

void MultiMessageQueue::AddMessageType(off_t _Type, int Priority) {
	pthread_mutex_lock(&mutex);

	map<off_t,TypeBookkeeping>::iterator itr = typeMap.find(_Type);
	if (itr!=typeMap.end()){
		if (_Type!=DieMessage::type) {
			WARNING("Type %ld already defined.\n", (unsigned long)_Type);
		}
	} else {
		TypeBookkeeping bk(Priority,_Type);
		typeMap.insert(pair<off_t,TypeBookkeeping>(_Type,bk));
	}

	pthread_mutex_unlock(&mutex);
}

void MultiMessageQueue::InsertMessage(Message& _Payload) {
	Message &Payload = _Payload;

	off_t _Type=Payload.Type();
	pthread_mutex_lock(&mutex);

	map<off_t,TypeBookkeeping>::iterator itr = typeMap.find(_Type);
    if (itr!=typeMap.end()){
        // we support this type, insert it
        TypeBookkeeping& bk = (*itr).second;
        InternalMessage msg(Payload, timestamp);
        bk.InsertMessage(msg);

    } else {
        // Message isn't registered, add it to unregistered list.
        InternalMessage msg(Payload, timestamp);
        unregisteredMessages.push_back(msg);
    }

    // increment timestamp to advance time
    timestamp++;

    // message in
    numMessages++;

    if(debug){
        printf("MSG IN %25s:%25s, Time=%4d.\n", dbgMsg, _Payload.TypeName(), timestamp);
    }

    pthread_cond_signal(&condVar); // signal the consumer that there is stuff in
    pthread_mutex_unlock(&mutex);
}

Message& MultiMessageQueue::RemoveMessage() {
	pthread_mutex_lock(&mutex);

	// do we have anything in? if not block
	while (numMessages == 0){
		pthread_cond_wait(&condVar, &mutex);
	}

	// now we know something is inside
	assert(numMessages!=0);

	// form the information for the decision function
	TypeInfo types[MAX_NUM_TYPES];
	int num = 0;

	for (map<off_t,TypeBookkeeping>::iterator itr = typeMap.begin();
				itr!=typeMap.end(); itr++){
		TypeBookkeeping& bk = (*itr).second;
		if (bk.GetTypeInfo(timestamp,&types[num])){
			// we have at least one message
			num++;
		}
	}

    InternalMessage msg;

    if( num > 0 ) {
        // If we have a registered message, call the decision function to figure out
        // what message to pop.
        // call the decision function to figure out what message to pop
        off_t typeRet = decFct(decProcessor, types, num);

        // get the message of type typeRet
        map<off_t,TypeBookkeeping>::iterator itr = typeMap.find(typeRet);
        assert(itr!=typeMap.end());

        TypeBookkeeping& bk = (*itr).second;
        msg = bk.NextMessage();
    }
    else {
        // Otherwise, we should have an unregistered message to send.
        FATALIF(unregisteredMessages.size() == 0, "numMessages > 0, but there were no messages to send.");

        msg = unregisteredMessages.front();
        unregisteredMessages.pop_front();
    }

	// message extracted
	numMessages--;

	if (debug == true){
		printf("MSG OUT%25s:%25s, Time=%4d, QSize=%3d.\n",
						dbgMsg, msg.GetPayload().TypeName(), msg.GetTimestamp(),
			numMessages);
	}

	pthread_mutex_unlock(&mutex);

	return msg.GetPayload();
}

int MultiMessageQueue::GetSize() {
	return numMessages;
}

off_t MultiMessageQueue::defaultDecisionFunction
(EventProcessorImp& _obj, TypeInfo* arrayTypeInfo, int num) {
	assert(num>=1); // we must have at least one message to be called

	// the default decision function just finds the highest priority message
	int minP=arrayTypeInfo[0].priority;
	int pos=0;
	for (int i=1; i<num; i++){
		if (arrayTypeInfo[i].priority<minP){
			minP=arrayTypeInfo[i].priority;
			pos=i;
		}
	}

	return arrayTypeInfo[pos].type;
}
