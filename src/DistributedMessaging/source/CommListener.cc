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
#include "CommListener.h"
#include "CommManager.h"
#include "CommunicationFramework.h"
#include "Errors.h"
#include "Logging.h"
#include "RemoteMacros.h"
#include "RemoteMessage.h"
#include "CommConfig.h"
#include "json.h"

#include <iostream>
#include <sstream>
#include <string>

using namespace std;


CommListenerImp::CommListenerImp( const HostAddress & addr )
{
    using websocketpp::lib::bind;
    using websocketpp::lib::placeholders::_1;
    using websocketpp::lib::placeholders::_2;

    // Open socket for listener (WebSocket)
    try {
        // Create a new server endpoint
        server = server_ptr(new ws_server());

        // Set up logging
        server->set_access_channels(websocketpp::log::alevel::none);
        server->set_error_channels(websocketpp::log::elevel::none);

        // Initialize ASIO
        server->init_asio();

        // Register handlers
        server->set_open_handler( bind(& CommListenerImp::OnConnect, this, _1) );
        server->set_close_handler( bind(& CommListenerImp::OnClose, this, _1) );
        server->set_message_handler( bind(& CommListenerImp::OnMessage, this, _1, _2) );

        // Listen on our port
        server->listen(addr.port);

        // Start the accept loop
        server->start_accept();
    }
    catch( const std::exception & e ) {
        ostringstream err;
        err << "CommListener: "
            << "Standard Exception received while attempting to set up websocket server: "
            << e.what() << endl;

        FATAL( err.str().c_str() );
    }
    catch( websocketpp::lib::error_code e ) {
        ostringstream err;
        err << "CommListener: "
            << "Websocket++ error code received while attempting to set up websocket server: "
            << e.message() << endl;

        FATAL( err.str().c_str() );
    }
    catch( ... ) {
        ostringstream err;
        err << "CommListener: "
            << "Unknown exception type received while attempting to set up websocket server" << endl;

        FATAL( err.str().c_str() );
    }
}

HostAddress CommListenerImp :: GetAddressFromHDL( connection_hdl hdl ) {
    connection_ptr ptr = server->get_con_from_hdl(hdl);

    return HostAddress( ptr->get_host(), ptr->get_port() );
}

/*
 *  Function called when the server receives a new message from a client.
 *
 *  The expected structure of the message payload is as follows:
 *
 *  {
 *      "type" : MessageType
 *      "msg":  depends on type
 *  }
 *
 *  NORMAL messages have the following form:
 *  {
 *      "source" : <HostAddress>,
 *      "dest" : <MailboxAddress>,
 *      "type" : <int>,
 *      "payload" : <json>
 *  }
 *
 *  HELLO messages have the following form:
 *  {
 *      "source"    : <HostAddress>
 *      "dest"      : <HostAddress>
 *  }
 */
void CommListenerImp::OnMessage( connection_hdl hdl, message_ptr msg ) {
    using comm::MessageType;

    const string & payload = msg->get_payload();

    Json::Reader jReader;
    Json::Value frame;

    bool valid = jReader.parse(payload, frame);

    if( valid ) {
        // TODO: Add error checking

        MessageType mType = static_cast<MessageType>(frame["type"].asUInt64());
        Json::Value & message = frame["msg"];
        if( mType == MessageType::NORMAL ) {
            HostAddress srcAddress;
            MailboxAddress destAddress;
            srcAddress.FromJson(message["source"]);
            destAddress.FromJson(message["dest"]);

            uint64_t msgType = message["type"].asUInt64();
            Json::Value & msgPayload = message["payload"];

            CommManager & commMan = CommManager::GetManager();
            EventProcessor destProc;
            int ret = commMan.GetEventProcessor( destAddress.mailbox, destProc );

            if( ret == 0 ) {
                // Send the message
                RemoteMessageContainer_Factory( destProc, srcAddress, destAddress, msgType, msgPayload );
            }
            else {
                COMM_LOG_ERR( "CommListener: "
                        "Received message for invalid mailbox %s",
                        destAddress.mailbox.c_str());
            }
        } // message is standard type
        else if( mType == MessageType::HELLO ) {
            HostAddress source;
            HostAddress dest;
            source.FromJson(message["source"]);
            dest.FromJson(message["dest"]);

            COMM_LOG_MSG("CommListener: Got Hello Message from host [%s]", source.str().c_str());

            connection_pair newConn( server, hdl );

            // Tell the CommManager about the new connection.
            CommManager & commMan = CommManager::GetManager();
            commMan.ConnectionOpened(source, newConn);
        } // message was a HELLO control message
        else {
            COMM_LOG_ERR(
                    "CommListener: "
                    "Received message containing invalid message type: %lu",
                    frame["type"].asUInt64()
                    );
        } // Invalid message type
    }
    else {
        // Message payload wasn't valid JSON.

        COMM_LOG_ERR(
            "CommListener: "
            "Received message containing invalid JSON Message: %s",
            payload.c_str());
    }
}

void CommListenerImp :: OnConnect( connection_hdl hdl ) {
    HostAddress addr = GetAddressFromHDL(hdl);
    COMM_LOG_MSG("CommListener: New connection from host [%s]", addr.str().c_str());

    activeConnections.insert(hdl);
}

void CommListenerImp :: OnClose( connection_hdl hdl ) {
    HostAddress addr = GetAddressFromHDL(hdl);
    COMM_LOG_MSG("CommListener: Closed connection from host [%s]", addr.str().c_str());

    auto iter = activeConnections.find(hdl);
    if( iter != activeConnections.end() )
        activeConnections.erase(iter);

    // Tell the CommManager that the connection has closed
    CommManager & commMan = CommManager::GetManager();
    commMan.ConnectionClosed(addr);
}

CommListenerImp::~CommListenerImp() {
    connection_list conns(activeConnections);
    std::string closeMsg("Server shutting down");
    websocketpp::lib::error_code ec;

    for( auto el : conns ) {
        server->close(el, websocketpp::close::status::going_away, closeMsg, ec);
        if( ec ) {
            COMM_LOG_ERR("CommListener: "
                        "Error while attempting to close a connection: %s",
                        ec.message().c_str()
                    );
        }
    }
}


int CommListenerImp::ProduceMessage(void){
    try {
        // Start the ASIO loop
        server->run();
    }
    catch( const std::exception & e ) {
        COMM_LOG_ERR("CommListener: "
                "Standard Exception received while running server: %s",
                e.what());
    }
    catch( websocketpp::lib::error_code e ) {
        cerr << "Websocket++ error code received while running websocket server: "
            << e.message() << endl;
    }
    catch( ... ) {
        cerr << "Unknown exception type received while running websocket server" << endl;
    }

    return -1;
}
