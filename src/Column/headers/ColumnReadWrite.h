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

#ifndef CR_H
#define CR_H

#include "Constants.h"
#include "Column.h"
#include "Iterator.h"

class Column;

class ColumnReadWrite {

	Iterator it;
	
public:

	// creates a column iterator for the given column... the requests for data that
	// are sent to the column are of size stepSize.  iterateMe is consumed.
	ColumnReadWrite (Column &iterateMe, int stepSize = COLUMN_ITERATOR_STEP);

	// This will iterate from [start, end)
	ColumnReadWrite (Column &iterateMe, int start, int end, int stepSize = COLUMN_ITERATOR_STEP);

	ColumnReadWrite () {};
	// destructor... if there is a column left in the ColumnReadWrite, it will be
	// destroyed as well
	~ColumnReadWrite ();

	// tells the iterator that we are done iterating... any new bytes written past
	// the end of the column will be included in the column from now on.  The column
	// is taken out of the iterator and put into iterateMe
	void Done (Column &iterateMe);

	// this is not needed as this interface is more like systems read and write
	// void Advance (int size);

	// add a new data object into the column at the current position, overwriting the
	// bytes that are already there.
	void write (char* data, int size);

	// Just make sure we have enough data in column and return the pointer. User is requested
	// not to modify the data of column
	void read (char* data, int size);

	// returns true if the object under the cursor has never been written to and so
	// it should not be read (it is undefined what happens if you read it)
	int AtUnwrittenByte ();

/**
  create deep copy from 'fromMe'. Everything is created new down the hierarchy,
  basically all iterator states are replicated, new Column and MMappedStorage is
  created with partial data + states copied. Storage is write only for this iterator.
  The storage (data) is copied only up to the current position of fromMe. This is used
  (for example) in the join, where you find a tuple that matches with more than one tuple
  on the RHS and so now you need to start making multiple copies of values.

  Here is the short usage,
    ColumnReadWrite<Type> myIter;
    // make sure fromMe.Done(col) is NOT called before making deep copy
    // after below line, myIter is ready to add more data using Insert(), since
    // it is pointing at the end of new replicated partial storage.
    myIter.CreateDeepCopy (fromMe); // myIter will contain write only storage
                                    // and ready to add more data using Insert()
    // Now start Inserting into myIter
*/
	void CreateDeepCopy (ColumnReadWrite& fromMe);

	void swap (ColumnReadWrite& swapMe);

	void MarkFragment ();

};

#endif
