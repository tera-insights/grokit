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
#ifndef _COMM_MANAGER_H_
#define _COMM_MANAGER_H_

#include <string>
#include <mutex>
#include <condition_variable>
#include <cinttypes>

#include "CommConfig.h"
#include "RemoteAddress.h"
#include "EventProcessor.h"
#include "ProxyEventProcessor.h"
#include "Swap.h"
#include "TwoWayList.h"
#include "EfficientMap.h"
#include "CommListener.h"
#include "CommSender.h"
#include "CommReceiver.h"

/**
  Container for all the data structures used for the communication management.
  Data structures:
  - communication listener
  - (mailbox --> event processor) map; mailboxes that event processors are signed up for
  - (HostAddress --> communication sender) map; what is the communication sender
  to a given host
  - (HostAddressService --> proxy event processor) map

  A CommSender object is created for each remote host the current host is
  communicating with (indexed on the address of the remote host).

  A ProxyEventProcessor is created for each (address, service) pair corresponding
  to a remote host. All proxy event processor for the same remote host but for
  different services use the same common CommSender object to send their messages.
  Thus, there is a single CommSender for a remote host that relays messages for
  multiple ProxyEventProcessor objects for different services.
  */
class CommManager {
    private:
        typedef comm::ws_client_pair ws_client_pair;
        typedef comm::ws_client_conn_ptr ws_client_conn_ptr;
        typedef comm::ws_server_pair ws_server_pair;

        //singleton instance
        static CommManager singleton;

        //block the copy constructor
        CommManager(CommManager&);

        //clean the expired data structures for dead senders and receivers
        int CleanDead();

    protected:
        //these are the members of the class used for management

        typedef Keyify<std::string> CString;
        typedef Keyify<int> CInt;
        typedef Keyify<off_t> COff_t;

        // Helpful typedefs for the mutexes used
        typedef std::mutex mutex_type;
        typedef std::lock_guard<mutex_type> scoped_lock_type;
        typedef std::unique_lock<mutex_type> unique_lock_type;

        typedef std::condition_variable condition_variable_type;
        typedef std::shared_ptr<condition_variable_type> condition_variable_ptr;
        typedef std::map<HostAddress, condition_variable_ptr> ConditionMap;

        //the access is protected by a mutex
        mutex_type m_mutex;

        // Main communication listener
        CommListener listener;

        // The address used by this machine.
        HostAddress myAddress;

        //receiving event processors, i.e. the event processors that do the
        //processing of the message
        typedef EfficientMap <CString, EventProcessor> ProcessorMap;
        ProcessorMap processors;

        //communication sender event processors
        typedef EfficientMap <HostAddress, CommSender> SenderMap;
        SenderMap senders;
        SenderMap deadSenders;
        ConditionMap senderSignals;

        // Keep track of receivers created to handle connections started by us
        typedef EfficientMap<HostAddress, CommReceiver> ReceiverMap;
        ReceiverMap receivers;
        ReceiverMap deadReceivers;

        typedef EfficientMap<HostAddress, ws_client_conn_ptr> ClientConnectionMap;
        typedef EfficientMap<HostAddress, ws_server_pair> ServerConnectionMap;
        ClientConnectionMap clientConnections;
        ServerConnectionMap serverConnections;

        // Condition variables for the clientConnections and serverConnections
        // maps.
        ConditionMap clientConditions;
        ConditionMap serverConditions;

        //communication senders proxy event processors
        //the only entity used to send remote messages
        typedef EfficientMap <MailboxAddress, ProxyEventProcessor> ProxySenderMap;
        ProxySenderMap proxySenders;

    public:
        //default constructor; initializes the manager
        CommManager(void);

        //function to get access to the singleton instance
        static CommManager& GetManager(void);

        //destructor
        virtual ~CommManager(void);

        //interface methods
        //start the communication manager
        int Start(uint16_t listenPort);

        //stop the communication manager
        int Stop(void);

        // Gets the address of this machine.
        HostAddress GetHostAddress(void) const;

        //find the event processor that is signed up for the given mailbox
        int GetEventProcessor(const std::string mailbox, EventProcessor& _processor);

        //find the proxy event processor in charge of sending messages to address for service
        int FindRemoteEventProcessor(const MailboxAddress & addr, EventProcessor& _where);

        // register who as the event processor for service
        int RegisterAsRemoteEventProcessor(EventProcessor& _who, const std::string & mailbox );

        // unregister the event processor for service
        int UnregisterRemoteEventProcessor(const std::string mailbox);

        //stop communication listener (it already stopped by itself)
        int StopListener();

        // Eliminate the sender (and receiver if it exists) for this address.
        //int EliminateConnection(const HostAddress & address );

        // A new connection has been opened by a remote host, so create a sender
        // to handle outgoing messages to that host if one does not already exist.
        void ConnectionOpened( const HostAddress & address, const ws_client_conn_ptr & connection );
        void ConnectionOpened( const HostAddress & address, const ws_server_pair & connection );

        // Get an connection for this address. Blocks until one is available.
        void GetConnection( const HostAddress & address, ws_client_conn_ptr & here );
        void GetConnection( const HostAddress & address, ws_server_pair & here );

        // An existing connection with a remote host has been closed.
        void ConnectionClosed( const HostAddress & address );
};

//inline methods
inline CommManager& CommManager::GetManager(void) {
    return singleton;
}

#endif // _COMM_MANAGER_H_
