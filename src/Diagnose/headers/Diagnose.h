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
/** Global diagnostic facilities in Datapath
*/

#ifndef _DIAGNOSE_H_
#define _DIAGNOSE_H_

#include <vector>
#include <algorithm>

#include "Timer.h" // for the global clock
#include "Errors.h"	//for FATALIF

extern Timer global_clock;

//==============================Added by: Praveen=============================================
//MessageData and MessageContainer; MessageContainer stores lots of MessageData objects.

//Message data container
//TimeStamp is passed via the global_clock
struct MessageData {
    long msgID;				//messageID, a kind of distributed counter
    const char* msgSender;  //sender of this message
    const char* msgDest;    //destination of this message
    const char* msgType;    //type of this message
    double msgTimeStamp;    //timestamp when this message is registered to the Diagnose

    //constructor
    MessageData(long id, const char* sender, const char* dest, const char* mtype, double timeStamp)
        : msgID(id), msgSender (sender), msgDest (dest), msgType (mtype), msgTimeStamp (timeStamp)
    {}
};

// Functor for deleting pointers in vector; MessageContainer.msgVector
template<class T>
struct DeleteVectorFntor
{    // Overloaded () operator.
    // This will be called by for_each() function.
    bool operator()(T x) const
    {   // Delete pointer.
        delete x;
        return true;
    }
};

//container for MessageData objects
class MessageContainer {
    private:
        std::vector<MessageData*> msgVector;

    public:

        MessageContainer(){};
        void swap(MessageContainer& other){ msgVector.swap(other.msgVector);};
        void Insert(MessageData * md){ msgVector.push_back(md);};
        MessageData* Get(int i){ return msgVector[i];};
        size_t Size(){ return msgVector.size();};
        void Clear(){//Empty the vector
            for_each(msgVector.begin(), msgVector.end(),   DeleteVectorFntor<MessageData*>());
            msgVector.clear();
        };
};

//============================================================================================

//Including here as containers defined above are used in the following header files.
#include "SqliteDumper.h"
#include "DumpRequestMessage.h"

//Main class
class Diagnose {

    private:
        bool DIAGNOSE_ON;	//if true: macro DIAGNOSE_ENTRY updates the Diagnose data structures otherwise the macro does nothing

    public:
        Diagnose();
        ~Diagnose();

        long onePassLimit;
        SqliteDumper sqliteDumper;
        volatile bool exclusion;  /* It's marked volatile, because it may change in a way
                                     which is not predictable by the compiler,
                                     here from a different thread. */
        volatile long counter;	//used as a distributed counter to provide an ID number to each message

        MessageContainer msgContainer;	 //MessageData object container, defined in ContainerTypes.h

        //The main program must call this method to set DIAGNOSE_ON and hence start collecting data for diagnosis. The flag exclusion is set to false.
        void StartDiagnose(void) {DIAGNOSE_ON = true; exclusion = false; counter=0; onePassLimit = 1000;};

        //The main program can call it to stop collecting data for diagnosis.
        void StopDiagnose(void) { DIAGNOSE_ON = false; counter = 0;};

        //a check if, DIAGNOSE_ON is true or false.
        bool IsDiagnose() { return DIAGNOSE_ON;};

};

extern Diagnose diagnose; //declared in Diagnose.cc as global

//__sync_bool_compare_and_swap(&(diagnose.exclusion), false, true ) == false);
//Macro to make an entry in MessageContainer. When it becomes full, it is sent to SqliteDumper to dump in SQLite.
//First check if DIAGNOSE_ON is set and accordingly updates the data structures.
//This macro is made thread safe otherwise there may be too many actors in the critical section.
//Uses spin lock (busy-wait) and not mutex as it is just updating the data structure. Using mutex may be costlier due to context switching.
//the flag exclusion is set to initial false in StartDiagnose() method.
#define DIAGNOSE_ENTRY(sender, dest, msgType)({																					\
        int id = -1;	\
        if(diagnose.IsDiagnose()) {																									\
        double ts = global_clock.GetTime(); 												\
        /* spin lock using atomic compare and set operation in <condition> */													\
        while(__sync_bool_compare_and_swap(&(diagnose.exclusion), false, true ) == false) { /* checking over and over */}		\
        id = diagnose.counter++;		\
        MessageData *mD = new MessageData(id, sender, dest, msgType, ts); FATALIF(!(mD), "Not Enough Memory.");	\
        diagnose.msgContainer.Insert(mD);																					\
        if(diagnose.msgContainer.Size()==diagnose.onePassLimit) DumpRequestMessage_Factory(diagnose.sqliteDumper, false, diagnose.msgContainer);	\
        diagnose.exclusion = false; /* TODO: think of using an atomic operation here*/											\
        }\
        id;												\
        })

typedef long DIAG_ID;

//Macro to complement previous macro to find out execution time of the block
#define DIAGNOSE_EXIT(id)																										\
    if(diagnose.IsDiagnose()) {																								\
        double ts = global_clock.GetTime(); 												\
        /* spin lock using atomic compare and set operation in <condition> */													\
        while(__sync_bool_compare_and_swap(&(diagnose.exclusion), false, true ) == false) { /* checking over and over */}		\
        MessageData *mD = new MessageData(id, NULL, NULL, NULL, ts); FATALIF(!(mD), "Not Enough Memory.");	\
        diagnose.msgContainer.Insert(mD);																					\
        if(diagnose.msgContainer.Size()==diagnose.onePassLimit) DumpRequestMessage_Factory(diagnose.sqliteDumper, false, diagnose.msgContainer);	\
        diagnose.exclusion = false; /* TODO: think of using an atomic operation here*/											\
    }

#endif // _DIAGNOSE_H_
