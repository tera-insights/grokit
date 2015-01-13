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

#include "EfficientIntToIntMap.h"

void EfficientIntToIntMap :: AllocateMoreMem () {

	// remember the current memory, so we can free it
	SwapifiedPointer temp (curMemoryBlock);
	allMemIveAllocated.Insert (temp);

	// and allocate the new memory
	curMemoryBlock = new Node[numInCurBlock * 2];
	numInCurBlock *= 2;
	numUsedInCurBlock = 0;
}

EfficientIntToIntMap :: ~EfficientIntToIntMap () {

	// delete all of the memory blocks we have recorded
	SwapifiedPointer temp;
	for (allMemIveAllocated.MoveToStart (); allMemIveAllocated.RightLength (); ) {
		allMemIveAllocated.Remove (temp);
		delete [] (Node *) temp;
	}

	// and delete the current memory block
	delete [] curMemoryBlock;
}

EfficientIntToIntMap :: EfficientIntToIntMap () {

	// get the memory block used to allocate nodes
	curMemoryBlock = new Node[32];
	numInCurBlock = 32;
	numUsedInCurBlock = 0;

	// get the random val we'll xor with
	xOrWithMe = lrand48 ();
	xOrWithMe <<= 32;
	xOrWithMe |= lrand48 ();

	// and set the root to null
	root = 0;
}

