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
#include "EventProcessor.h"
#include "EventProcessorImp.h"
#include "DistributedCounter.h"
#include "Errors.h"
#include "Swap.h"
#include "Message.h"


EventProcessor::EventProcessor(void) noexcept {
    // we just make the pointer null to make sure
    // none of the functions have any effect
    evProc = nullptr;
    numCopies = new DistributedCounter(1);
    noDelete = false;
}

EventProcessor::EventProcessor(const EventProcessor& o) noexcept:
    evProc(o.evProc),
    numCopies(o.numCopies),
    noDelete(o.noDelete)
{
    numCopies->Increment(1);
}

EventProcessor::EventProcessor(EventProcessor && o) noexcept :
    evProc(o.evProc),
    numCopies(o.numCopies),
    noDelete(o.noDelete)
{
    // Invalidate the moved processor
    o.evProc = nullptr;
    o.numCopies = new DistributedCounter(1);
    o.noDelete = false;
}

EventProcessor & EventProcessor::operator = (const EventProcessor & o ) {
    copy(o);
    return *this;
}

EventProcessor::EventProcessor(EventProcessorImp* const obj) :
    evProc(obj),
    numCopies(new DistributedCounter(1)),
    noDelete(true)
{
}

void EventProcessor::ProcessMessage(Message& msg) {
    if (evProc != nullptr) {
        evProc->ProcessMessage(msg);
    }
    else {
        FATAL("Message sent to an unititialized EventProcessor.");
        assert(1==2);
    }
}

bool EventProcessor::ForkAndSpin(int node) {
    if (evProc != nullptr) {
        return evProc->ForkAndSpin(node);
    }

    return false;
}

void EventProcessor::Seppuku(void) {
    if (evProc != nullptr) {
        evProc->Seppuku();
    }
}

void EventProcessor::WaitForProcessorDeath(void) {
    if (evProc != nullptr) {
        evProc->WaitForProcessorDeath();
    }
}

EventProcessor::~EventProcessor() {
    Clear();
}

void EventProcessor::Clear(void) {
    // is this the last copy?
    if (numCopies->Decrement(1) == 0) {
        // distroy the event processor and the counter
        if (evProc != nullptr && !noDelete)
            delete evProc;

        delete numCopies;
    }
}

void EventProcessor::swap(EventProcessor& other) {
    SWAP_ASSIGN(evProc, other.evProc);
    SWAP_ASSIGN(numCopies, other.numCopies);
    SWAP_ASSIGN(noDelete, other.noDelete);
}

void EventProcessor::copy(const EventProcessor& other) {
    Clear();

    // put in the new content
    evProc = other.evProc;
    numCopies = other.numCopies;
    noDelete = other.noDelete;
    numCopies->Increment(1);
}

