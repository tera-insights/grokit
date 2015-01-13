// Copyright 2013 Tera Insights, LLC. All Rights Reserved.

#include "CommReceiver.h"
#include "CommManager.h"
#include "CommunicationFramework.h"
#include "Errors.h"
#include "Logging.h"
#include "RemoteMacros.h"
#include "RemoteMessage.h"
#include "CommConfig.h"
#include "SerializeJson.h"

#include <string>
#include <sstream>

#include <ctime>

using namespace std;

CommReceiverImp::CommReceiverImp( const HostAddress & addr ) :
    remoteAddr(addr),
    conn_fails(0),
    active(false)
{
    using websocketpp::lib::bind;
    using websocketpp::lib::placeholders::_1;
    using websocketpp::lib::placeholders::_2;

    try {
        // Create a new client endpoint
        client = client_ptr(new ws_client());

        // Set up logging
        client->set_access_channels(websocketpp::log::alevel::none);
        client->set_error_channels(websocketpp::log::elevel::none);

        // Initialize ASIO
        client->init_asio();

        // Register handlers
        client->set_open_handler( bind(& CommReceiverImp::OnConnect, this, _1) );
        client->set_fail_handler( bind(& CommReceiverImp::OnFail, this, _1) );
        client->set_close_handler( bind(& CommReceiverImp::OnClose, this, _1) );
        client->set_message_handler( bind(& CommReceiverImp::OnMessage, this, _1, _2) );

        Connect(remoteAddr);

    }
    catch( const std::exception & e ) {
        ostringstream err;
        err << "CommReceiver: "
            << "Standard Exception received while attempting to set up websocket client: "
            << e.what() << endl;

        FATAL( err.str().c_str() );
    }
    catch( websocketpp::lib::error_code e ) {
        ostringstream err;
        err << "CommReceiver: "
            << "Websocket++ error code received while attempting to set up websocket client: "
            << e.message() << endl;

        FATAL( err.str().c_str() );
    }
    catch( ... ) {
        ostringstream err;
        err << "CommReceiver: "
            << "Unknown exception type received while attempting to set up websocket client" << endl;

        FATAL( err.str().c_str() );
    }
}

CommReceiverImp :: ~CommReceiverImp() {
    if( active ) {
        std::string closeReason("Client shutting down");
        websocketpp::lib::error_code ec;
        connection->close(websocketpp::close::status::going_away, closeReason, ec);

        if( ec )
            COMM_LOG_ERR("CommReceiver: [%s] "
                        "Error occured while attempting to close connection: %s",
                        remoteAddr.str().c_str(), ec.message().c_str()
                    );

        client->stop();
    }
}

HostAddress CommReceiverImp :: GetAddressFromHDL( connection_hdl hdl ) const {
    connection_ptr ptr = client->get_con_from_hdl(hdl);

    return HostAddress( ptr->get_host(), ptr->get_port() );
}

string CommReceiverImp :: GetURI( const HostAddress & addr ) const {
    ostringstream uri;

    if( client->is_secure() ) {
        uri << "wss://";
    }
    else {
        uri << "ws://";
    }

    uri << addr.hostname << ":" << addr.port;
    uri << "/grokit";

    return uri.str();
}

void CommReceiverImp :: Connect( const HostAddress & addr ) {
    string uri = GetURI(addr);
    websocketpp::lib::error_code ec;

    connection = client->get_connection(uri, ec);

    if( ec ) {
        throw ec;
    }

    // Connection is asynchronous
    client->connect(connection);
}

int CommReceiverImp :: ProduceMessage( void ) {
    try {
        // Start the ASIO loop
        active = true;
        client->run();
    }
    catch( const std::exception & e ) {
        COMM_LOG_ERR("CommReceiver: [%s] "
                "Standard Exception received while running client: %s",
                remoteAddr.str().c_str(), e.what());
    }
    catch( websocketpp::lib::error_code e ) {
        COMM_LOG_ERR("CommReceiver: [%s] ",
                "Websocket++ error code received while running websocket client: %s",
                remoteAddr.str().c_str(), e.message().c_str());
    }
    catch( ... ) {
        COMM_LOG_ERR("CommReceiver: [%s] "
                "Unknown exception type received while running websocket client",
                remoteAddr.str().c_str());
    }

    if( active ) {
        // Reset the client so that we can run again
        client->reset();

        // Connection has failed, attempt to reconnect
        Wait();
        Connect(remoteAddr);

        return 0;
    }
    else {
        // Connection no longer active, just exit
        return -1;
    }

}

void CommReceiverImp :: Wait(void) const {
    // Only wait if more than 1 failure. If we've only failed once, immediately
    // attempt to reconnect.
    // Otherwise, wait for conn_fails * wait_interval,
    // where wait_interval is currently 5 seconds for no real reason.
    if( conn_fails > 1 ) {
        const time_t secsToSleep = static_cast<time_t>( (conn_fails - 1) * 5);
        timespec toSleep = { secsToSleep, 0 };
        timespec remaining;

        while( nanosleep(&toSleep, &remaining) != 0 ) {
            toSleep = remaining;
        }
    }
}

// Inform the CommManager about the new connection so it can create/update the
// corresponding CommSender
void CommReceiverImp :: OnConnect( connection_hdl hdl ) {
    HostAddress addr = GetAddressFromHDL(hdl);
    COMM_LOG_MSG("CommReciever: New connection from host [%s]", addr.str().c_str());

    CommManager & commMan = CommManager::GetManager();

    // Send a message to the remote host to let them know who we are.
    websocketpp::lib::error_code ec;
    Json::Value frame(Json::objectValue);
    frame["type"] = static_cast<Json::UInt64>(comm::MessageType::HELLO);
    ToJson(commMan.GetHostAddress(), frame["msg"]["source"]);
    ToJson(addr, frame["msg"]["dest"]);

    Json::FastWriter writer;
    std::string data = writer.write(frame);
    client->send(hdl, data, websocketpp::frame::opcode::TEXT, ec);
    COMM_LOG_MSG("CommReciever: [%s] sending hello message: %s", remoteAddr.str().c_str(), data.c_str());

    if( ec ) {
        COMM_LOG_ERR("CommReceiver: [%s] failed to send hello message.", remoteAddr.str().c_str());
    }

    conn_fails = 0;

    commMan.ConnectionOpened( addr, connection );
}

void CommReceiverImp :: OnFail( connection_hdl hdl ) {
    conn_fails ++;

    HostAddress addr = GetAddressFromHDL(hdl);
    COMM_LOG_ERR("CommReceiver: Failed to connect to host [%s], # of failures in a row: %llu",
            addr.str().c_str(), conn_fails);

    // Stop the client so we can try to reconnect
    client->stop();
}

void CommReceiverImp :: OnMessage( connection_hdl hdl, message_ptr msg ) {
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
                COMM_LOG_ERR( "Received message for invalid mailbox %s", destAddress.mailbox.c_str());
            }
        } // message is standard type
        else if( mType == MessageType::HELLO ) {
            COMM_LOG_ERR(
                    "CommReceiver: [%s] "
                    "Received hello message, only Listeners should get one of these!",
                    remoteAddr.str().c_str()
                    );

        } // message was a HELLO control message
        else {
            COMM_LOG_ERR(
                    "CommReceiver: "
                    "Received message containing invalid message type: %lu",
                    frame["type"].asUInt64()
                    );
        } // Invalid message type

    }
    else {
        // Message payload wasn't valid JSON.

        COMM_LOG_ERR(
            "CommReceiver: "
            "Received message containing invalid JSON Message: %s",
            payload.c_str());
    }
}

// Inform the CommManager about the connection dropping.
void CommReceiverImp :: OnClose( connection_hdl hdl ) {
    HostAddress addr = GetAddressFromHDL(hdl);
    COMM_LOG_MSG("CommReceiver: Closed connection from host [%s]", addr.str().c_str());

    CommManager & commMan = CommManager::GetManager();
    commMan.ConnectionClosed( addr );

    if( connection->get_remote_close_code() == websocketpp::close::status::going_away ||
            connection->get_local_close_code() == websocketpp::close::status::going_away ) {
        // Connection was closed normally.
        active = false;
    }
    else {
        // Connection was closed abnormally
        conn_fails++;
    }

    // Stop the client so that we'll try to reconnect.
    client->stop();
}
