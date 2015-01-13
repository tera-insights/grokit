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
#include "EventGenerator.h"
#include "EventGeneratorImp.h"
#include "DistributedCounter.h"
#include "Swap.h"


EventGenerator::EventGenerator(void) {
	// we just make the pointer null to make sure
	// none of the functions have any effect
	evGen = NULL;
	numCopies = new DistributedCounter(1);
	noDelete = false;
}

EventGenerator::~EventGenerator(void){
	// is this the last copy?
	if (numCopies->Decrement(1)==0){
		// distroy the event generator and the counter
		if (evGen!=NULL && !noDelete)
			delete evGen;

		delete numCopies;
	}
}

void EventGenerator::Run(){
	if (evGen!=NULL)
		evGen->Run();
}

void EventGenerator::Kill(){
	if (evGen!=NULL)
		evGen->Kill();
}

void EventGenerator::swap(EventGenerator& other){
	SWAP_ASSIGN(evGen, other.evGen);
	SWAP_ASSIGN(numCopies, other.numCopies);
	SWAP_ASSIGN(noDelete, other.noDelete);
}

void EventGenerator::copy(EventGenerator& other){
	// same code as the destructor
	if (numCopies->Decrement(1)==0){
		// distroy the event generator and the counter
		if (evGen!=NULL && !noDelete)
			delete evGen;

		delete numCopies;
	}

	// put in the new content
	evGen=other.evGen;
	numCopies=other.numCopies;
	noDelete=other.noDelete;
	numCopies->Increment(1);
}
