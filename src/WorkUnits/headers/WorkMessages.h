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
/**
	This header file contains the messages used by Workers.
*/








#ifndef _WORK_MESSAGES_H_
#define _WORK_MESSAGES_H_

#include "MessageMacros.h"
#include "Message.h"
#include "Worker.h"
#include "WorkUnit.h"


//////////// WORK TO DO MESSAGE /////////////

/** Message sent by ExecutionEngine to request work to be done by a Worker.

		Arguments:
			workUnit: work to be done
			myName: handle for the Worker
*/

class WorkToDoMessage : public Message {
public:
  //members

  WorkUnit workUnit;
  Worker myName;

private:
  //constructor
  WorkToDoMessage(WorkUnit& _workUnit, Worker& _myName )
  {
    // swap members
    workUnit.swap(_workUnit);
    myName.swap(_myName);
  }

  // private default constructor so nobody can build this stuff
  WorkToDoMessage(void);

public:
	//destructor
	virtual ~WorkToDoMessage() {}

  // type
  static const off_t type=0x7a845660bc5c4a0dULL
 ;
  virtual off_t Type(void){ return 0x7a845660bc5c4a0dULL
 ; }
	virtual const char* TypeName(void){ return "WorkToDoMessage"; }

  // friend declarations
  friend void WorkToDoMessage_Factory(EventProcessor& dest ,WorkUnit& _workUnit, Worker& _myName);

};

// Factory function to build WorkToDoMessage
inline void WorkToDoMessage_Factory(EventProcessor& dest ,WorkUnit& _workUnit, Worker& _myName){
  Message* msg = new WorkToDoMessage(_workUnit, _myName);
  dest.ProcessMessage(*msg);
}



//////////// WORK DONE MESSAGE /////////////

/** Message sent by Worker to the ExecutionEngine when the work is done.

		Arguments:
			timeS: how long did it take to execute the work
			runThis: work that has been done
			sentFrom: Worker who did the work and sent the message
*/

class WorkDoneMessage : public Message {
public:
  //members

  double timeS;
  WorkUnit runThis;
  Worker sentFrom;

private:
  //constructor
  WorkDoneMessage(double _timeS, WorkUnit& _runThis, Worker& _sentFrom ):
    // copy constructed members
    timeS(_timeS)
  {
    // swap members
    runThis.swap(_runThis);
    sentFrom.swap(_sentFrom);
  }

  // private default constructor so nobody can build this stuff
  WorkDoneMessage(void);

public:
	//destructor
	virtual ~WorkDoneMessage() {}

  // type
  static const off_t type=0x427aadeb0aa37a16ULL
 ;
  virtual off_t Type(void){ return 0x427aadeb0aa37a16ULL
 ; }
	virtual const char* TypeName(void){ return "WorkDoneMessage"; }

  // friend declarations
  friend void WorkDoneMessage_Factory(EventProcessor& dest ,double _timeS, WorkUnit& _runThis, Worker& _sentFrom);

};

// Factory function to build WorkDoneMessage
inline void WorkDoneMessage_Factory(EventProcessor& dest ,double _timeS, WorkUnit& _runThis, Worker& _sentFrom){
  Message* msg = new WorkDoneMessage(_timeS, _runThis, _sentFrom);
  dest.ProcessMessage(*msg);
}


#endif // _WORK_MESSAGES_H_
