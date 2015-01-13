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
#ifndef _COMM_SENDER_H_
#define _COMM_SENDER_H_

#include "MessageMacros.h"
#include "EventProcessor.h"
#include "EventProcessorImp.h"
#include "RemoteAddress.h"
#include "CommConfig.h"

#include <string>
#include <memory>

/**
  Communication sender opens a connection with a remote host and sends all the
  messages destined to that host from the local machine.
  The address of the remote host is passed at construction time.
  The remote host is always contacted on a pre-configured port.
  */
class CommSenderImp : public EventProcessorImp {
    private:
        // Typedefs
        typedef comm::ws_client_pair ws_client_pair;
        typedef comm::ws_server_pair ws_server_pair;
        typedef comm::connection_hdl connection_hdl;
        typedef comm::ws_server_ptr ws_server_ptr;
        typedef comm::ws_client_ptr ws_client_ptr;
        typedef comm::ws_client_conn_ptr conn_ptr;

        // The address of the remote machine.
        HostAddress addr;

        // We use one of these two connections depending on how we were constructed
        conn_ptr client_conn;
        ws_server_pair server_conn;

        // Whether or not we are attached to a server.
        // If we are, use the server_conn, otherwise use the client_conn.
        // I could have done this in a nicer way, but it probably isn't worth the trouble.
        // Blame Alin.
        bool attachedToServer;

        // Whether or not the sender is active
        bool isActive;

        // Helper method to send a message
        void SendMessage(const std::string & data);

    public:
        //constructor & destructor
        CommSenderImp( const HostAddress & _addr, const conn_ptr & _conn );
        CommSenderImp( const HostAddress & _addr, const ws_server_pair & _conn );
        virtual ~CommSenderImp();

        //close the connection
        int Close();

        //check to see if the sender is active (it has a connection to the remote host)
        bool IsActive() {return isActive;}

        // message handler for all remote messages
        MESSAGE_HANDLER_DECLARATION(ProcessRemoteMessage);
};

class CommSender : public EventProcessor {
    private:
        typedef comm::ws_client_conn_ptr conn_ptr;
        typedef comm::ws_server_pair ws_server_pair;

    public:
        //empty constructor needed for initialization
        CommSender() {}
        CommSender(const HostAddress & _remoteMachine, const conn_ptr & _conn ) {
            evProc = new CommSenderImp(_remoteMachine, _conn );
        }
        CommSender(const HostAddress & _remoteMachine, const ws_server_pair & _conn ) {
            evProc = new CommSenderImp(_remoteMachine, _conn );
        }
        virtual ~CommSender() {}

        //return the internal EventProcessorImp needed for ProxyEventProcessor build
        EventProcessorImp* GetEvProc() { return dynamic_cast<EventProcessorImp*>(evProc); }

        //close the socket
        int Close() {
            CommSenderImp* sender = dynamic_cast<CommSenderImp*>(evProc);
            return sender->Close();
        }

        //check to see if the sender is active (it has a connection to the remote host)
        bool IsActive() {
            CommSenderImp* sender = dynamic_cast<CommSenderImp*>(evProc);
            return sender->IsActive();
        }
};

#endif // _COMM_SENDER_H_
