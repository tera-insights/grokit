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
#ifndef _COMM_CONFIG_H_
#define _COMM_CONFIG_H_

#include "Config.h"
#include "Logging.h"

#include <cinttypes>
#include <memory>
#include <utility>

#include <websocketpp/config/asio.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/server.hpp>

#ifdef DEBUG_COMM_FRAMEWORK
    #define COMM_LOG_ERR(msg, args...) FATAL(msg, args)
    #define COMM_LOG_MSG(msg, args...) WARNING(msg, args)
#else
    #define COMM_LOG_ERR(msg, args...) LOG_ENTRY_P(4, msg, args)
    #define COMM_LOG_MSG(msg, args...) LOG_ENTRY_P(6, msg, args)
#endif

namespace comm {
    // Port for communcation listener
    CONSTEXPR uint16_t LISTEN_PORT_WS = 11111;

    // Message queue ID for killer generator
    CONSTEXPR off_t KILL_MSG_QUEUE_ID = 1111;

    // Kill message type for killer generator message queue
    CONSTEXPR off_t KILL_MSG_TYPE = 999;

    // Port for local frontend
    CONSTEXPR uint16_t FRONTEND_PORT = 9000;

    /***** Overall websocket++ typedefs *****/
    typedef websocketpp::connection_hdl connection_hdl;

    /***** Server Typedefs *****/
    typedef websocketpp::config::asio ws_config_server;
    typedef websocketpp::server<ws_config_server> ws_server;
    typedef std::shared_ptr<ws_server> ws_server_ptr;
    typedef std::pair<ws_server_ptr, connection_hdl> ws_server_pair;

    /***** Client Typedefs *****/
    typedef websocketpp::config::asio_client ws_config_client;
    typedef websocketpp::client<ws_config_server> ws_client;
    typedef std::shared_ptr<ws_client> ws_client_ptr;
    typedef ws_client::connection_ptr ws_client_conn_ptr;
    typedef std::pair<ws_client_ptr, connection_hdl> ws_client_pair;

    enum class MessageType {
        NORMAL  = 0,
        HELLO   = 1
    };
}

#endif // _COMM_CONFIG_H_
