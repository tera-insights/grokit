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
#include <unistd.h>

#include <string>
#include <climits>

#include "EventProcessor.h"
#include "ProxyEventProcessor.h"
#include "CommManager.h"
#include "CommListener.h"
#include "CommSender.h"
#include "Message.h"
#include "RemoteMacros.h"
#include "CommConfig.h"
#include "Errors.h"

#include "InefficientMap.h"
#include "TwoWayList.h"
#include "InefficientMap.cc"
#include "TwoWayList.cc"

using namespace std;

//default constructor
CommManager::CommManager()
{
}

//destructor
CommManager::~CommManager()
{
    // WARNING("Destructor Begin.");
    // stop will take care of most stuff
    Stop();

    // destroy the listener
    CommListener _listener;
    listener.swap(_listener);

// WARNING("Destructor End.");
}

HostAddress CommManager::GetHostAddress(void) const
{
    return myAddress;
}

//start the communication manager
int CommManager::Start(uint16_t listenPort)
{
    cout << "Start communication manager." << endl;

    //initialize members

    //get the name of the host
    char buffer[HOST_NAME_MAX + 1];
    gethostname(buffer, HOST_NAME_MAX + 1);
    string machineName(buffer);

    myAddress = HostAddress(machineName, listenPort);

    //initialize the listener
    CommListener _listener(myAddress);
    listener.swap(_listener);

    //start the communication listener
    scoped_lock_type guard(m_mutex);
    listener.Run();

    return 0;
}


//stop the communication manager
int CommManager::Stop(void)
{
//  WARNING("Stop Begin.");

//clean the dead
    CleanDead();

    {
        // Guard access to members
        scoped_lock_type guard(m_mutex);

        //senders and receivers are special; they need to be killed
        senders.MoveToStart();
        while (!senders.AtEnd()) {
            CommSender& _sender = senders.CurrentData();
            KillEvProc(_sender);

            senders.Advance();
        }

        processors.Clear();
        proxySenders.Clear();
        senders.Clear();

        //stop the listener
        listener.Kill();
    }

    cout << "Stop communication manager." << endl;

    //  WARNING("Stop End.");

    return 0;
}

//find the event processor that is signed up for the given mailbox
int CommManager::GetEventProcessor(const std::string _mailbox, EventProcessor& _processor)
{
    int ret = 0;

    CString mailbox(_mailbox);

    //  WARNING("GetEventProcessor Begin.");

    // access to the data structure is guarded
    {
        scoped_lock_type guard(m_mutex);

        int isIn = processors.IsThere(mailbox);
        if (isIn) {
            //if the proxy event processor already exists, return it and exit
            _processor.copy(processors.Find(mailbox));

            // WARNING("GetEventProcessor End.");

            return 0;
        }
    }

    //the processor for the asked service does not exist

    WARNING("There is no event processor registered for mailbox %s!", _mailbox.c_str());

    return -1;
}


//find the proxy event processor in charge of sending messages to address for service
int CommManager::FindRemoteEventProcessor(const MailboxAddress & addr, EventProcessor& _where)
{
    //WARNING("FindRemoteEventProcessor Begin.");

    MailboxAddress remoteHost = addr;

    //clean the dead
    CleanDead();

    // access to the data structure is guarded
    unique_lock_type guard(m_mutex);

    int isIn = proxySenders.IsThere(remoteHost);
    if (isIn) {
        //if the proxy event processor already exists, return it and exit
        _where.copy(proxySenders.Find(remoteHost));

        // WARNING("FindRemoteEventProcessor End.");

        return 0;
    }

    //this is the first time the proxy is called, thus we need to create it
    //there are three cases:
    //  - a CommSender already exists for the given address --> do not create it
    //  - a CommReceiver already exists for the address --> wait for Sender to be created
    //  - no CommReceiver exists for the address --> create one and wait for sender
    HostAddress address = remoteHost.host;

    if( senderSignals.find(address) == senderSignals.end() ) {
        senderSignals[address] = condition_variable_ptr(new condition_variable_type);
    }

    if( ! senders.IsThere(address) && ! receivers.IsThere(address) ) {
        // Need to create a receiver to start the connection.
        HostAddress key = address;
        CommReceiver rec(address);
        rec.Run();

        receivers.Insert(key, rec);
    }

    // While the sender isn't available, wait for it to become available.
    while( ! senders.IsThere(address) ) {
        senderSignals[address]->wait(guard);
    }

    //the comm sender is already part of the communication framework
    CommSender& commSender = senders.Find(address);
    CommSender tmpSender;
    tmpSender.copy(commSender);
    ProxyEventProcessor newProxySender(tmpSender, remoteHost);
    newProxySender.ForkAndSpin();
    _where.copy(newProxySender);


    proxySenders.Insert(remoteHost, newProxySender);

    //  WARNING("FindRemoteEventProcessor End.");



    // WARNING("FindRemoteEventProcessor End.");

    return 0;
}


//register who as the event processor for service
int CommManager::RegisterAsRemoteEventProcessor(EventProcessor& _who, const string & _mailbox)
{
    CString mailbox(_mailbox);

    // WARNING("RegisterAsRemoteEventProcessor Begin.");

    //access to the data structure is guarded
    scoped_lock_type guard(mutex);

    int isIn = processors.IsThere(mailbox);
    if (isIn) {

        WARNING("There already exists an event processor registered for mailbox %s!", _mailbox.c_str());
        return -1;
    }

    //the processor for the asked service does not exist, add it
    EventProcessor addMe;
    addMe.copy(_who);
    processors.Insert(mailbox, addMe);


    // WARNING("RegisterAsRemoteEventProcessor End.");

    return 0;
}


//unregister the event processor for service
int CommManager::UnregisterRemoteEventProcessor(const string _mailbox)
{
    CString mailbox(_mailbox);
    CString _key;
    EventProcessor _data;

    // WARNING("UnregisterRemoteEventProcessor Begin.");

    scoped_lock_type guard(mutex);

    processors.Remove(mailbox, _key, _data);

    // WARNING("UnregisterRemoteEventProcessor End.");

    return 0;
}


//stop communication listener (it already stopped by itself)
int CommManager::StopListener()
{
//	WARNING("StopListener Begin.");
//	WARNING("StopListener End.");

    return 0;
}


/*
//eliminate comm sender that does not function normally anymore
//in addition, all the proxy event processors that use the sender are eliminated
int CommManager::EliminateSender(const HostAddress& _remoteMachine) {
    HostAddress key(_remoteMachine);
    HostAddress _source;
    CommSender _sender;

    //clean the dead
    CleanDead();

    // WARNING("EliminateSender Begin.");

    { // begin scoped lock
        scoped_lock_type guard(m_mutex);

        //first eliminate the proxy senders that use the sender
        proxySenders.MoveToStart();
        while (!proxySenders.AtEnd())
        {
            MailboxAddress& psKey = proxySenders.CurrentKey();
            if (psKey.host == _remoteMachine) {
                //eliminate the proxy event processor
                MailboxAddress key1(psKey);
                MailboxAddress _source1;
                ProxyEventProcessor _proxy;

                // FIXME: Maybe we shouldn't kill the proxies?
                // We should probably tell them that their sender is dead instead,
                // and let them know in the future if a connection is reestablished.
                proxySenders.Remove(key1, _source1, _proxy);
                KillEvProc(_proxy);
            }
            else {
                proxySenders.Advance();
            }
        }

        senders.Remove(key, _source, _sender);
    } // end scoped lock

    //close the socket corresponding to the sender
    _sender.Close();

    { // begin scoped lock
        scoped_lock_type guard(m_mutex);

        //then eliminate the sender in order to allow its correct delete
        deadSenders.Insert(_source, _sender);
    } // end scoped lock

    // WARNING("EliminateSender End.");

    return 0;
}
*/

//clean the expired data structures for dead senders and receivers
int CommManager::CleanDead()
{
    scoped_lock_type guard(m_mutex);

    //kill the dead senders
    deadSenders.MoveToStart();
    while (!deadSenders.AtEnd()) {
        CommSender& _sender = deadSenders.CurrentData();
        DieMessage_Factory(_sender);

        deadSenders.Advance();
    }

    deadSenders.Clear();

    deadReceivers.MoveToStart();
    while(!deadReceivers.AtEnd()) {
        CommReceiver & _receiver = deadReceivers.CurrentData();
        _receiver.Kill();

        deadReceivers.Advance();
    }

    deadReceivers.Clear();

    return 0;
}

void CommManager :: ConnectionOpened( const HostAddress & address, const ws_client_conn_ptr & connection )
{
    COMM_LOG_MSG("CommManager: New connection to host [%s] received from receiver.", address.str().c_str());
    unique_lock_type guard(m_mutex);

    // Put the connection in the clietnConnections map
    HostAddress key = address;
    auto value = connection;
    clientConnections.Insert(key, value);

    // If there isn't a condition variable for this host, add one.
    if( clientConditions.find(address) == clientConditions.end() ) {
        clientConditions[address] = condition_variable_ptr(new condition_variable_type());
    }

    // Notify anyone waiting on the connection.
    clientConditions[address]->notify_all();

    // If we don't have a sender for this address, make one using the connection.
    if( ! senders.IsThere(address) ) {
        HostAddress key = address;
        CommSender newSender(address, connection);
        newSender.ForkAndSpin();

        senders.Insert(key, newSender);

        if( senderSignals.find(address) != senderSignals.end() ) {
            // There are people waiting on a sender for this address.
            // Let them know it's available.
            senderSignals[address]->notify_all();
        }
    }
}

void CommManager :: ConnectionOpened( const HostAddress & address, const ws_server_pair & connection )
{
    COMM_LOG_MSG("CommManager: New connection to host [%s] received from listener.", address.str().c_str());
    unique_lock_type guard(m_mutex);

    // Put the connection in the connections map
    HostAddress key = address;
    ws_server_pair value = connection;
    serverConnections.Insert(key, value);

    // If there isn't a condition variable for this host, add one.
    if( serverConditions.find(address) == serverConditions.end() ) {
        serverConditions[address] = condition_variable_ptr(new condition_variable_type());
    }

    // Notify anyone waiting on the connection.
    serverConditions[address]->notify_all();

    // If we don't have a sender for this address, make one using the connection.
    if( ! senders.IsThere(address) ) {
        HostAddress key = address;
        CommSender newSender(address, connection);
        newSender.ForkAndSpin();

        senders.Insert(key, newSender);

        if( senderSignals.find(address) != senderSignals.end() ) {
            // There are people waiting on a sender for this address.
            // Let them know it's available.
            senderSignals[address]->notify_all();
        }
    }
}

void CommManager :: ConnectionClosed( const HostAddress & address )
{
    COMM_LOG_MSG("CommManager: Connection to address %s closed", address.str().c_str());
    unique_lock_type(m_mutex);

    // If there was a client connection for this address, delete it
    if( clientConnections.IsThere(address) ) {
        HostAddress key;
        ws_client_conn_ptr value;
        clientConnections.Remove(address, key, value);
    }

    if( serverConnections.IsThere(address) ) {
        HostAddress key;
        ws_server_pair value;
        serverConnections.Remove(address, key, value);
    }
}

void CommManager :: GetConnection( const HostAddress & address, ws_client_conn_ptr & here )
{
    unique_lock_type guard(m_mutex);

    while( !clientConnections.IsThere(address) ) {
        clientConditions[address]->wait(guard);
    }

    ws_client_conn_ptr tmp = clientConnections.Find(address);
    here.swap(tmp);
}

void CommManager :: GetConnection( const HostAddress & address, ws_server_pair & here )
{
    unique_lock_type guard(m_mutex);

    while( !serverConnections.IsThere(address) ) {
        serverConditions[address]->wait(guard);
    }

    ws_server_pair tmp = serverConnections.Find(address);
    here.swap(tmp);
}
