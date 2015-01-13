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
#ifndef _EXECENGINE_H_
#define _EXECENGINE_H_

#include <string>

// include the base class definition
#include "EventProcessor.h"

// include the implementation definition
#include "ExecEngineImp.h"

/** Class to provide an interface to ExecEngineImp class.

  See ExecEngineImp.h for a description of the functions
  and behavior of the class
  */
class ExecEngine : public EventProcessor {

    protected:

        friend class WayPointImp;

        // these functions are called by the WayPointImp class to actually send messages... they should NOT
        // be called by anyone else.  If a particular waypoint type wants to send a message, it should make
        // a call to the WayPointImp class and not call these functions directly

        void SendHoppingDataMsg( HoppingDataMsg &msg ) {
            ExecEngineImp *temp = dynamic_cast <ExecEngineImp *> (evProc);
            temp->SendHoppingDataMsg(msg);
        }

        void SendHoppingDownstreamMsg (HoppingDownstreamMsg &msg) {
            ExecEngineImp *temp = dynamic_cast <ExecEngineImp *> (evProc);
            temp->SendHoppingDownstreamMsg (msg);
        }

        void SendHoppingUpstreamMsg (HoppingUpstreamMsg &msg) {
            ExecEngineImp *temp = dynamic_cast <ExecEngineImp *> (evProc);
            temp->SendHoppingUpstreamMsg (msg);
        }

        void SendAckMsg (QueryExitContainer &myContainer, HistoryList &history) {
            ExecEngineImp *temp = dynamic_cast <ExecEngineImp *> (evProc);
            temp->SendAckMsg (myContainer, history);
        }

        void SendDropMsg (QueryExitContainer &myContainer, HistoryList &history) {
            ExecEngineImp *temp = dynamic_cast <ExecEngineImp *> (evProc);
            temp->SendDropMsg (myContainer, history);
        }

        void SendDirectMsg (DirectMsg &msg) {
            ExecEngineImp *temp = dynamic_cast <ExecEngineImp *> (evProc);
            temp->SendDirectMsg (msg);
        }

        // this one is similar to the above, except that it gives back a work token... it should also not be
        // called directly
        void GiveBackToken (GenericWorkToken &returnVal) {
            ExecEngineImp *temp = dynamic_cast <ExecEngineImp *> (evProc);
            temp->GiveBackToken (returnVal);
        }

        // similar situation... these request tokens
        // If we have higher priority guys waiting, just ignore this
        int RequestTokenImmediate (WayPointID &myID, off_t requestType, GenericWorkToken &returnVal, int priority = 2) {
            ExecEngineImp *temp = dynamic_cast <ExecEngineImp *> (evProc);
            return temp->RequestTokenImmediate (myID, requestType, returnVal, priority);
        }

        void RequestTokenDelayOK (WayPointID &myID, off_t requestType, int priority = 2) {
            ExecEngineImp *temp = dynamic_cast <ExecEngineImp *> (evProc);
            temp->RequestTokenDelayOK (myID, requestType, priority);
        }

        void Debugg(void);

        int GetPriorityCutoff (off_t requestType) {
            ExecEngineImp *temp = dynamic_cast <ExecEngineImp *> (evProc);
            return temp->GetPriorityCutoff (requestType);
        }

        void SetPriorityCutoff (off_t requestType, int priority) {
            ExecEngineImp *temp = dynamic_cast <ExecEngineImp *> (evProc);
            temp->SetPriorityCutoff (requestType, priority);
        }

        void ReclaimToken (GenericWorkToken &putResHere) {
            ExecEngineImp *temp = dynamic_cast <ExecEngineImp *> (evProc);
            temp->ReclaimToken (putResHere);
        }

        // Service methods
        void SendServiceReply( ServiceData& reply ) {
            ExecEngineImp *temp = dynamic_cast<ExecEngineImp *>( evProc );
            temp->SendServiceReply(reply);
        }

        void SendServiceInfo( std::string & service, std::string & status, Json::Value& data ) {
            ExecEngineImp *temp = dynamic_cast<ExecEngineImp *>( evProc );
            temp->SendServiceInfo(service, status, data);
        }

    public:

        // default constructor
        ExecEngine(const std::string & mailbox){
            evProc = new ExecEngineImp (mailbox);
        }

        // the virtual destructor
        virtual ~ExecEngine(){}

};


// this is the system's execution engine
extern ExecEngine executionEngine;

#endif // _EXECENGINE_H_
