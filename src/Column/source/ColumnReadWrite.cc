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

#include "ColumnReadWrite.h"
using namespace std;
#include <iostream>
#include <assert.h>
#include <string.h>

// min bytes to get length is randomly 8 by default, but interface need to be updated later by asking the object itself
ColumnReadWrite :: ColumnReadWrite (Column &iterateMe, int stepSize) : it (iterateMe, 8, stepSize) {

	if (it.IsInvalid ())
		return;

	// If column is blank and valid
	if (it.GetFirstInvalidByte() == 0)
		return;

	// This should ensure min bytes to get length of bytes
	it.EnsureFirstObjectSpace(8);
}

ColumnReadWrite :: ColumnReadWrite (Column &iterateMe, int fragmentStart, int fragmentEnd, int stepSize) : it (iterateMe, fragmentStart, fragmentEnd, 8, stepSize) {

	if (it.IsInvalid ())
		return;

	// If column is blank and valid
	if (it.GetFirstInvalidByte() == 0)
		return;

	// This should ensure min bytes to get length of bytes
	it.EnsureFirstObjectSpace(8);
}

inline int ColumnReadWrite :: AtUnwrittenByte () {
	return it.AtUnwrittenByte ();
}

ColumnReadWrite :: ~ColumnReadWrite () {
} 

void ColumnReadWrite :: Done (Column &iterateMe) {

	it.Done (iterateMe);
}

void ColumnReadWrite :: write (char* data, int size) {

	if (it.IsInvalid ())
		return;

	// find some optimal way to remove it later
	assert(it.IsWriteOnly() == true);	

	it.SetObjLen (size);

	it.EnsureWriteSpace ();

	// and write the object
	memcpy(it.GetData(), data, size);

	it.Advance();
}

void ColumnReadWrite :: read (char* data, int size) {

	if (it.IsInvalid ())
		return;

	it.EnsureSpace (size, size);

	memcpy(data, it.GetData(), size);

	it.SetObjLen (size);
	it.Advance();
}

void ColumnReadWrite :: CreateDeepCopy (ColumnReadWrite& fromMe) {

	// do shallow copy first
	//memmove (this, &fromMe, sizeof (ColumnReadWrite<DataType>)); dont work
	it.CreateDeepCopy (fromMe.it);
}

void ColumnReadWrite :: swap (ColumnReadWrite& swapMe) {

	it.swap (swapMe.it);
}

void ColumnReadWrite :: MarkFragment () {

	it.MarkFragment();
}
