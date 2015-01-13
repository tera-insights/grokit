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
#ifndef _COMM_LISTENER_H_
#define _COMM_LISTENER_H_

#include "CommConfig.h"

#include "EventGenerator.h"
#include "EventGeneratorImp.h"
#include "RemoteAddress.h"

#include <set>
#include <memory>

/**
  Class responsible for accepting connections by the current machine
  specified by the (name:port) pair. Upon receiving a connection request from
  a different machine, the communication manager is invoked to register a
  communication receiver.
  */
class CommListenerImp : public EventGeneratorImp {
    private:
        // Typedefs
        typedef comm::connection_hdl connection_hdl;

        typedef comm::ws_server ws_server;
        typedef comm::ws_server_ptr server_ptr;
        typedef comm::ws_server_pair connection_pair;
        typedef comm::ws_server::connection_ptr connection_ptr;
        typedef comm::ws_server::endpoint_type::message_ptr message_ptr;

        typedef std::set<connection_hdl> connection_list;

        // The server used by this listener
        server_ptr server;

        // The list of open connections
        connection_list activeConnections;

        // Functions to register with the server to handle events
        void OnConnect( connection_hdl hdl );
        void OnMessage( connection_hdl hdl, message_ptr msg );
        void OnClose( connection_hdl hdl );

        // Translates a connection handle to a host address.
        // This is only safe to use inside one of the handler functions.
        HostAddress GetAddressFromHDL( connection_hdl hdl );

    public:
        //constructor & destructor
        CommListenerImp(const HostAddress & _addr);
        virtual ~CommListenerImp();

        //method inherited from the base class to generate messages for communication manager
        virtual int ProduceMessage(void);
};

class CommListener : public EventGenerator {
    public:
        //empty constructor needed for initialization
        CommListener() {}
        virtual ~CommListener() {}

        //constructor with implementation provided
        CommListener(const HostAddress & _addr) {
            evGen = new CommListenerImp(_addr);
        }
};


#endif // _COMM_LISTENER_H_
