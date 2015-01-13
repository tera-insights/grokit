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
#ifndef _COMMUNICATION_FRAMEWORK_H_
#define _COMMUNICATION_FRAMEWORK_H_

#include <string>
#include <cinttypes>

#include "EventProcessor.h"
#include "RemoteAddress.h"

/**
  Function has to be called before using any part of the communication
  framework. It should be called from the main program after the metadata
  database is set but before use of communication.
  */
int StartCommunicationFramework( uint16_t listenPort );

/**
  Stop the communication framework.
  */
int StopCommunicationFramework(void);

/**
  Wait for the communication framework death.
  That is, wait until the killer event processor is killed.
  */
int WaitCommunicationFrameworkDeath(void);

/**
  Function to get a handle to a remote EventProcessor machine.
  - address is the name of the machine (as recognized by TCP/IP)
  - service is the service identifier
  - the corresponding ProxyEventProcessor is returned in where
  Return an error code if unsuccessful, 0 otherwise.
  */
int FindRemoteEventProcessor(const MailboxAddress & addr,
        EventProcessor& where);

/**
  Function to register a remote message processing capability.
  Return 0 if successful or an error code in the case of any error.
  */
int RegisterAsRemoteEventProcessor(EventProcessor& who, const std::string & mailbox);

/**
  Function to unregister a remote message processing capability.
  Return 0 if successful or an error code in the case of any error.
  */
int UnregisterRemoteEventProcessor(const std::string mailbox);

/**
 *  Function to get the address of the local machine.
 */
int GetHomeAddress( HostAddress & putHere );

/**
 *  Function to get the address of the frontend server.
 */
int GetFrontendAddress( HostAddress & putHere );

/**
  Function to kill the sending facilities with a remote host.
  Return 0 if successful or an error code in the case of any error.

  Current unsupported
  */
//extern int EndCommunication(const HostAddress& address);

#endif // _COMMUNICATION_FRAMEWORK_H_
