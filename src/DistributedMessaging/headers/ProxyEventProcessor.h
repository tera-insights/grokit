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
#ifndef _PROXY_EVENT_PROCESSOR_
#define _PROXY_EVENT_PROCESSOR_

#include "EventProcessor.h"
#include "EventProcessorImp.h"
#include "Numa.h"
#include "Message.h"
#include "RemoteAddress.h"

class ProxyEventProcessorImp : public EventProcessorImp {
    private:
        // The address of the actor we are representing
        MailboxAddress addr;

        // The address of this machine.
        HostAddress myAddr;

        // The CommSender we used to send data to the remote actor
        EventProcessor commSender;

    public:
        // Constructor & destructor
        ProxyEventProcessorImp(void) : EventProcessorImp() { }

        ProxyEventProcessorImp(EventProcessor & __commSender, MailboxAddress & _addr);

        virtual ~ProxyEventProcessorImp() { }

        // Override the default message handler
        static void RemoteSendHandler(EventProcessorImp & evProc, Message & msg);
};

/**
  This is a special handle for sending messages remotely.
  It only communicates with the special event processor in the
  communication framework.
*/
class ProxyEventProcessor : public EventProcessor {
    public:
        // constructor & destructor
        ProxyEventProcessor(void):EventProcessor(){ }

        ProxyEventProcessor(EventProcessor & _commSender, MailboxAddress & _addr) {
            evProc = new ProxyEventProcessorImp(_commSender, _addr);
        }

        virtual ~ProxyEventProcessor(){};
};


#endif // _PROXY_EVENT_PROCESSOR_
