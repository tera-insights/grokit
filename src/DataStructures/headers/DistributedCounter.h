//
//  Copyright 2012 Alin Dobra and Christopher Jermaine
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0 // //  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
#ifndef _DISTRIBUTED_COUNTER_H
#define _DISTRIBUTED_COUNTER_H

#include <assert.h>
#include "Errors.h"
#include <atomic>
#include <mutex>
#include <cinttypes>
#include "Swapify.h"

/**
  Class to implement a distributed counter that can safely be incremented and decreemnted
  by multiple threads.
  */
class DistributedCounter {
    private:
        std::atomic<int64_t> counter;

        // FIXME: This should be removed. The re-using of the counter's mutex
        // was not very clean.
        std::mutex m_lock;

        DistributedCounter(DistributedCounter&) = delete; // disable copy constructor
        DistributedCounter& operator= (const DistributedCounter&) = delete; // same for assignment

    public:
        // constructor. Parameter is initial value
        DistributedCounter(int64_t _initial = 0);

        // destructor
        virtual ~DistributedCounter(void);

        // increment and return the count atomically
        int64_t Increment(int64_t _increment);

        // look at the current value
        int64_t GetCounter(void);

        // decrement and return the count atomicaly
        int64_t Decrement(int64_t _decrement);

        // reuse of the mutex to provide mutual exclusion
        void Lock(void);
        void Unlock(void);
};


inline  DistributedCounter::DistributedCounter(int64_t _initial):
    counter(_initial)
{
}

inline DistributedCounter::~DistributedCounter(void){
}

inline int64_t DistributedCounter::Increment(int64_t _increment){
    return counter.fetch_add(_increment, std::memory_order_seq_cst) + _increment;
}

inline int64_t DistributedCounter::GetCounter(void){
    return counter.load(std::memory_order_seq_cst);
}


inline int64_t DistributedCounter::Decrement(int64_t _decrement){
    int64_t ret = counter.fetch_sub(_decrement, std::memory_order_seq_cst) - _decrement;

    return ret;
}

inline void DistributedCounter::Lock(void){
    m_lock.lock();
}

inline void DistributedCounter::Unlock(void){
    m_lock.unlock();
}

typedef Swapify<DistributedCounter*> SwapifiedDCptr;

#endif // _DISTRIBUTED_COUNTER_H
