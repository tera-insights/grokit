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

#ifndef RANDENG
#define RANDENG

#include "HashEntry.h"

#include <math.h>
#include <stdlib.h>

// this is used to generate a random series of ranges for the hash table merge.
// It turns out that just going from the bottom to the top is a problem, because
// there is too much blocking
struct RandEngine {

	// this is the list of random numbers to choose
	unsigned *nums;

	// data used to compute the ranges
	int numChunks;
	HT_INDEX_TYPE numEntries;
	int last;

	RandEngine () {
		nums = NULL;
	}

	// reset the random range generator
	void Reset (int numChunksIn, HT_INDEX_TYPE numEntriesIn) {
		delete [] nums;
		numChunks = numChunksIn;
		if (numChunks == 0)
			numChunks = 1;

		numEntries = numEntriesIn;
		nums = new unsigned[numChunks];
		last = 0;
		nums[0] = 0;

		for (int i = 0; i < numChunks; i++) {
			nums[i] = i;
		}

		int n = numChunks;
		while (n > 1) {
			n--;
			int k = lrand48 () % (n + 1);
			int tmp = nums[k];
			nums[k] = nums[n];
			nums[n] = tmp;
		}
	}

	// obtain a random range
	void Next (HT_INDEX_TYPE &low, HT_INDEX_TYPE &high) {
		int index = nums[last];
		low = (HT_INDEX_TYPE) rint (numEntries * (double) index / numChunks);
		high = (HT_INDEX_TYPE) rint (numEntries * (double) (index + 1) / numChunks);
		last++;
	}

	int IsDone () {
		return (last >= numChunks);
	}
};

#endif
