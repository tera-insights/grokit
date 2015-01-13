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
#ifndef _SPEED_CONTROL_H_
#define _SPEED_CONTROL_H_

#include "Bitstring.h"
#include "Logging.h" // for global clock
#include "QueryID.h"

/** Auxiliary class that encapsulates per query decisions
http://www.faqs.org/rfcs/rfc5348.html
*/

class SpeedQ {
    //???????
    // time internal at which we should produce the next chunk
    //default 28.5chunks/sec, or ~35ms per chunk = 0.035 seconds per chunk
    double prodInterval;
    // time at which chunk  is produced
    double lastProduce;
    long long int count;

    public:

    // all interfaces mimic the SpeedCtrl class
    // the extra argument is the time when it happened
    void Reset(double time);
    //every Ack, prodInterval is decreased by 10%, thus producing more chunks per sec
    void Ack(double time);
    //every Drop, prodInterval is increased by 20%, thus producing less chunks per sec
    void Drop(double time);

    // returns true if a chunk for this query can be produced, false otherwise
    // The system WILL produce the chunk (otherwise why ask) so bookkeeping can use this
    bool CanProduce(double time);

    double GetInterval(void);

};

/** Class that can determine the speed at which chunk production can work */

class SpeedCtrl {
    // vector of speed controllers, one for each possible query.
    SpeedQ sVec[BITS_PER_WORD];

    // kill the copy constructor
    SpeedCtrl(const SpeedCtrl&);
    SpeedCtrl& operator=(const SpeedCtrl&);
    public:

    SpeedCtrl();
    // reset some of the parts
    void Reset();
    // ReceivedAck
    void Ack(int i);
    // Received Drop
    void Drop(int i);

    /* Compute the set of queries for thich we can produce a chunk
       at this time
       */
    QueryIDSet Produce(QueryIDSet candidate);
    bool CanProduce(int i);
    double GetInterval(int i);

};

/***************** INLINE FUNCTIONS ************************/
inline SpeedCtrl::SpeedCtrl(){

}

inline void SpeedCtrl::Reset(){
    double time = global_clock.GetTime();
    for (unsigned int i = 0; i < BITS_PER_WORD; i++) {
        //        if (qrys.IsMember (i)) {
        sVec[i].Reset(time);
        //        }
    }
}

inline void SpeedCtrl::Ack(int i){
    double time = global_clock.GetTime();
    //    for (unsigned int i = 0; i < BITS_PER_WORD; i++) {
    //        if (qrys.IsMember (i)) {
    sVec[i].Ack(time);
    //        }
    //    }
}

inline void SpeedCtrl::Drop(int i){
    double time = global_clock.GetTime();
    //    for (unsigned int i = 0; i < BITS_PER_WORD; i++) {
    //        if (qrys.IsMember (i)) {
    sVec[i].Drop(time);
    //        }
    //    }
}

inline bool SpeedCtrl::CanProduce(int i){
    double time = global_clock.GetTime();
    //    for (unsigned int i = 0; i < BITS_PER_WORD; i++) {
    //        if (qrys.IsMember (i)) {
    return sVec[i].CanProduce(time);
    //        }
    //    }
    //    return false;
}

inline double SpeedCtrl::GetInterval(int i){
    double time = global_clock.GetTime();
    //    for (unsigned int i = 0; i < BITS_PER_WORD; i++) {
    //        if (qrys.IsMember (i)) {
    return sVec[i].GetInterval();
    //        }
    //    }
    //    return 0;
}

inline     QueryIDSet SpeedCtrl::Produce(QueryIDSet candidate){
    double time = global_clock.GetTime();
    QueryIDSet rez;
    //    for (unsigned int i = 0; i < BITS_PER_WORD; i++) {
    //        if (candidate.IsMember (i)) {
    //            if (sVec[i].CanProduce(time)){
    //                rez.AddMember(i);
    //            }
    //        }
    //    }
    return rez;
}

inline void SpeedQ::Reset(double time){
    prodInterval = 0.03;
    lastProduce = time;
    count = 0;
}

inline void SpeedQ::Ack(double time){

    if(count == 0 ){
        lastProduce = time;
    }
    else if(count == 1){
        prodInterval = time - lastProduce;
    }
    else {
        if(prodInterval > 0){
            double currentInterval = time - lastProduce;
            prodInterval -= (currentInterval - prodInterval)/count;
        }
    }
    count++;
}

inline void SpeedQ::Drop(double time){
    prodInterval += prodInterval*0.50;
    lastProduce = time;
}

inline double SpeedQ::GetInterval(){
    return prodInterval;
}

inline bool SpeedQ::CanProduce(double time){
    if((time-lastProduce) >= prodInterval){
        return true;
    }
    else{
        return false;
    }
}

#endif //  _SPEED_CONTROL_H_
