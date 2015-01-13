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
#include <string>
#include <iostream>
#include <stdio.h>
#include <memory>

#include "CommConfig.h"
#include "CommunicationFramework.h"
#include "CommManager.h"
#include "KillFramework.h"


using namespace std;


//initialize the singleton instance of communication manager
//this actually calls the default constructor that initializes the member variables
CommManager CommManager::singleton;

//killer variables
KillProcessor *killProc = NULL;
KillGenerator *killGen = NULL;


/**
  Function has to be called before using any part of the communication
  framework. It should be called from the main program after the metadata
  database is set but before use of communication.
  */
int StartCommunicationFramework(uint16_t listenPort) {
    cout << "Start communication framework." << endl;

    //start the killers
    //killProc = new KillProcessor();
    //killGen = new KillGenerator(killProc);

    //killProc->ForkAndSpin();
    //killGen->Run();

    //get an instance to the communication manager
    CommManager& manager = CommManager::GetManager();
    int res = manager.Start(listenPort);

    return res;
}

/**
  Stop the communication framework.
  */
int StopCommunicationFramework(void) {
    //get an instance to the communication manager
    CommManager& manager = CommManager::GetManager();
    int res = manager.Stop();

    //delete killProc;
    //delete killGen;

    cout << "Stop communication framework." << endl;

    return res;
}

/**
  Wait for the communication framework death.
  That is, wait until the killer event processor is killed.
  */
int WaitCommunicationFrameworkDeath(void) {
    killProc->WaitForProcessorDeath();

    cout << "Communication framework killed." << endl;

    return 0;
}

/**
  Function to get a handle to a remote EventProcessor machine.
  - address is the name of the machine (as recognized by TCP/IP)
  - service is the service identifier
  - the corresponding ProxyEventProcessor is returned in where
  Return an error code if unsuccessful, 0 otherwise.
  */
int FindRemoteEventProcessor(const MailboxAddress & address,
        EventProcessor& where) {
    //get an instance to the communication manager
    CommManager& manager = CommManager::GetManager();
    int res = manager.FindRemoteEventProcessor(address, where);

    return res;
}

/**
  Function to register a remote message processing capability.
  Return 0 if successful or an error code in the case of any error.
  */
int RegisterAsRemoteEventProcessor(EventProcessor& who,
        const std::string & mailbox) {
    //get an instance to the communication manager
    CommManager& manager = CommManager::GetManager();
    int res = manager.RegisterAsRemoteEventProcessor(who, mailbox);

    return res;
}

/**
  Function to unregister a remote message processing capability.
  Return 0 if successful or an error code in the case of any error.
  */
int UnregisterRemoteEventProcessor(const std::string mailbox) {
    //get an instance to the communication manager
    CommManager& manager = CommManager::GetManager();
    int res = manager.UnregisterRemoteEventProcessor(mailbox);

    return res;
}

int GetHomeAddress( HostAddress & putHere ) {
    CommManager & manager = CommManager::GetManager();
    putHere = manager.GetHostAddress();
    return 0;
}

int GetFrontendAddress( HostAddress & putHere ) {
    HostAddress localAddr;
    GetHomeAddress(localAddr);
    putHere = HostAddress(localAddr.hostname, comm::FRONTEND_PORT );
    return 0;
}

/**
  Function to kill the sending facilities with a remote host.
  Return 0 if successful or an error code in the case of any error.
  */
/*
   int EndCommunication(const HostAddress& address) {
//get an instance to the communication manager
CommManager& manager = CommManager::GetManager();
int res = manager.EliminateSender(address);

return res;
}
*/
