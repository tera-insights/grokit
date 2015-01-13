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

friend class Column;

private:

DistributedCounter* refCount; // reference counter

static DistributedCounter idCount; // id conter
//used to give all shallow copies of the same object the same ID.

int myID; // the id of this instance
protected:

	// does book-keeping so that *this will be treated as a shallow copy of makeCopyOfMe
	void SetCopyOf (ColumnStorage &makeCopyOfMe);

	// returns the unique ID of this storage
	int GetVersionID ();

	// tellse the storage to do the book-keeping necessary so it will not be treated
	// as a shallow copy of anyone
	void SetLoneCopy ();

// Function to clean content if last copy
void Clean(void);
