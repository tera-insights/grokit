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
#include "RemoteMessage.h"
#include "CommSender.h"
#include "CommConfig.h"
#include "Errors.h"
#include "Logging.h"
#include "CommManager.h"

#include <iostream>
#include <sstream>
#include <string>
#include <functional>
#include <thread>
#include <ctime>

using namespace std;

CommSenderImp::CommSenderImp(const HostAddress & _remoteMachine,
        const conn_ptr & _conn ):
    addr(_remoteMachine),
    client_conn(_conn),
    attachedToServer(false),
    isActive(false)
#ifdef DEBUG_EVPROC
    , EventProcessorImp(true, "CommSender") // comment to remove debug
#endif
{
    COMM_LOG_MSG("Starting Client CommSender for address %s", addr.str().c_str());

    isActive = true;

    RegisterMessageProcessor(RemoteMessage::type, &ProcessRemoteMessage, 1 /*priority*/);
}

CommSenderImp::CommSenderImp(const HostAddress & _remoteMachine,
        const ws_server_pair & _conn ):
    addr(_remoteMachine),
    server_conn(_conn),
    attachedToServer(true),
    isActive(false)
#ifdef DEBUG_EVPROC
    , EventProcessorImp(true, "CommSender") // comment to remove debug
#endif
{
    COMM_LOG_MSG("Starting Server CommSender for address %s", addr.str().c_str());

    isActive = true;

    RegisterMessageProcessor(RemoteMessage::type, &ProcessRemoteMessage, 1 /*priority*/);
}

void CommSenderImp :: SendMessage( const std::string & data ) {
    bool sent = false;
    while( !sent ) {
        websocketpp::lib::error_code ec;

        if( attachedToServer ) {
            ws_server_ptr & endpoint = server_conn.first;
            connection_hdl & handle = server_conn.second;

            endpoint->send(handle, data, websocketpp::frame::opcode::TEXT, ec);
        }
        else {
            ec = client_conn->send(data);
        }

        if( ec ) {
            COMM_LOG_ERR(
                    "CommSender [%s] "
                    "Error while attempting to send message: %s",
                    addr.str().c_str(), ec.message().c_str());

            // Get a new connection
            // wait 1 second before attempting to get a new connection.
            timespec toSleep = { 1, 0 };
            timespec remaining;

            while( nanosleep(&toSleep, &remaining) != 0 ) {
                toSleep = remaining;
            }

            CommManager & cMan = CommManager::GetManager();
            if( attachedToServer ) {
                cMan.GetConnection(addr, server_conn);
            }
            else {
                cMan.GetConnection(addr, client_conn);
            }
        }
        else {
            sent = true;
        }
    }
}

MESSAGE_HANDLER_DEFINITION_BEGIN(CommSenderImp, ProcessRemoteMessage, RemoteMessage){

    // Serialize the remote message.
    Json::Value toSend;
    toSend["type"] = static_cast<Json::UInt64>(comm::MessageType::NORMAL);
    msg.ToJson(toSend["msg"]);

    // Convert to a string
    Json::FastWriter writer;
    std::string sData = writer.write(toSend);

    evProc.SendMessage(sData);


}MESSAGE_HANDLER_DEFINITION_END

CommSenderImp::~CommSenderImp() {
    // close connection
    Close();
}

int CommSenderImp::Close() {
    isActive = false;

    return 0;
}

