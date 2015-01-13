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
#include "ColumnStorage.h"
#include "Errors.h"

DistributedCounter ColumnStorage :: idCount(0); // initialize the id counter at 0

int ColumnStorage :: IsLoneCopy () {
	FATALIF(refCount->GetCounter()==0, "Why counter is zero !!!");
	return (refCount->GetCounter() == 1);
}

ColumnStorage :: ColumnStorage () {
	refCount = new DistributedCounter(1); // 1 copy
	myID = idCount.Increment(1); // next id
}

ColumnStorage :: ~ColumnStorage () {
	Clean();
}

void ColumnStorage :: Clean(void){
	if (refCount->Decrement(1) == 0){
		// last copy, delete everything
		delete refCount;
	}
}

void ColumnStorage :: SetCopyOf (ColumnStorage &makeCopyOfMe) {

	// deallocate everything if we are the only copy
	Clean();

	// and copy all of this guy's book-keeping info
	refCount = makeCopyOfMe.refCount;
	refCount->Increment(1); // I am  copy as well
	myID = makeCopyOfMe.myID;
}

int ColumnStorage :: GetVersionID () {
	return myID;
}

void ColumnStorage :: SetLoneCopy () {

	if (!IsLoneCopy ()) {

		// mark that this guy is no longer a copy of anyone else
		Clean();
		refCount = new DistributedCounter(1);
		myID = idCount.Increment(1); // next id
	}
}

