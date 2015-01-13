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
#include "ProxyEventProcessor.h"
#include "EventProcessorImp.h"
#include "Message.h"
#include "RemoteMessage.h"
#include "EventProcessor.h"
#include "Swap.h"
#include "CommManager.h"

#include <iostream>

using namespace std;

ProxyEventProcessorImp :: ProxyEventProcessorImp( EventProcessor & __commSender, MailboxAddress & _addr ):
    addr(_addr)
{
    commSender.swap(__commSender);
    RegisterDefaultMessageProcessor(&ProxyEventProcessorImp::RemoteSendHandler);

    CommManager & cMan = CommManager::GetManager();
    myAddr = cMan.GetHostAddress();
}

void ProxyEventProcessorImp :: RemoteSendHandler( EventProcessorImp & evProc, Message & msg ) {

    ProxyEventProcessorImp & actProc = dynamic_cast<ProxyEventProcessorImp&>(evProc);

    MailboxAddress destAddr = actProc.addr;
    HostAddress srcAddr = actProc.myAddr;
    EventProcessor & commSender = actProc.commSender;

    // Serialize the message to JSON and create a new RemoteMessage to send to the CommSender.
    Json::Value payload;
    msg.ToJson(payload);
    off_t type = msg.Type();

    RemoteMessage_Factory(commSender, srcAddr, destAddr, type, payload);
}

