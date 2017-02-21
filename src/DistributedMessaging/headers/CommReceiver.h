// Copyright 2013 Tera Insights, LLC. All Rights Reserved.

#ifndef _COMM_RECEIVER_H_
#define _COMM_RECEIVER_H_

#include "CommConfig.h"

#include "EventGenerator.h"
#include "EventGeneratorImp.h"
#include "RemoteAddress.h"

#include <string>
#include <mutex>
#include <condition_variable>
#include <atomic>

/**
 * Class responsible for starting new connections to remote machines and then
 * receiving messages sent on the line.
 */
class CommReceiverImp : public EventGeneratorImp {
    public:
        // Typedefs
        typedef comm::connection_hdl connection_hdl;

        typedef comm::ws_client ws_client;
        typedef comm::ws_client_ptr client_ptr;
        typedef comm::ws_client_pair connection_pair;
        typedef comm::ws_client::connection_ptr connection_ptr;
        typedef comm::ws_client::endpoint_type::message_ptr message_ptr;

    private:

        // The client used by this receiver
        client_ptr client;

        // The connection being used by this receiver
        connection_ptr connection;

        // Address that this receiver connects to
        HostAddress remoteAddr;
CommReceiver
        // The number of times the connection has failed in a row. Used to determine
        // how long to wait before attempting to re-establish the connection.
        size_t conn_fails;

        // Whether or not the connection is active
        std::atomic<bool> active;

        // Functions to register with the server to handle events
        void OnConnect( connection_hdl hdl );
        void OnFail( connection_hdl );
        void OnMessage( connection_hdl hdl, message_ptr msg );
        void OnClose( connection_hdl hdl );

        // Translates a connection handle to a host address.
        // This is only safe to use inside one of the handler functions.
        HostAddress GetAddressFromHDL( connection_hdl hdl ) const;

        // Gets the URI to connect to on the host
        std::string GetURI( const HostAddress & addr ) const;

        // Connect to the given host.
        void Connect( const HostAddress & addr );

        // Wait for a certain amount of time before attempting to reconnect.
        // The wait time will depend upon the number of fails.
        void Wait(void) const;

    public:
        //constructor & destructor
        CommReceiverImp(const HostAddress & _addr);
        virtual ~CommReceiverImp();

        //method inherited from the base class to generate messages for communication manager
        virtual int ProduceMessage(void);
};

class CommReceiver : public EventGenerator {
    typedef comm::ws_client_pair connection_pair;

    public:
        //empty constructor needed for initialization
        CommReceiver() {}
        virtual ~CommReceiver() {}

        //constructor with implementation provided
        CommReceiver(const HostAddress & _addr) {
            evGen = new CommReceiverImp(_addr);
        }

        /*
        virtual connection_pair Getconnection(void) const {
            CommRecieverImp * gen = dynamic_cast<CommRecieverImp*>(evGen);
            FATALIF(gen == NULL, "Tried to get connection from non-CommReceiverImp");
            return gen->GetConnection();
        }
        */
};

#endif // _COMM_RECEIVER_H_
