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

using namespace std;
#include <iostream>
#include <assert.h>
#include "Iterator.h"
#include "Errors.h"
#include "Swap.h"

Iterator :: Iterator (Column &iterateMe,
        uint64_t minByteToGetLength,
        uint64_t stepSize) :
    myData (NULL),
    bytesToRequest (stepSize),
    curPosInColumn (0),
    firstInvalidByte (0),
    objLen (0),
    colLength (0),
    isWriteOnly (false),
    myMinByteToGetLength (minByteToGetLength),
    isInValid (false)
{

    if (!iterateMe.IsValid()) {
        isInValid = true;
        bytesToRequest = 0;
        myMinByteToGetLength = 0;
        return;
    }

    // process the input
    myColumn.swap (iterateMe);

    isWriteOnly = myColumn.IsWriteMode ();

    // find the length of the column
    colLength = myColumn.GetColLength ();

    // if we essentially have an empty column, then don't get the first object
    if (colLength < minByteToGetLength)
        return;

    // get enough storage to get the object length
    firstInvalidByte = stepSize;
    myData = myColumn.GetNewData (curPosInColumn, firstInvalidByte);
}

Iterator :: Iterator (Column &iterateMe,
        uint64_t fragmentStart,
        uint64_t fragmentEnd,
        uint64_t minByteToGetLength,
        uint64_t stepSize) :
    myData (NULL),
    bytesToRequest (stepSize),
    curPosInColumn (0),
    firstInvalidByte (0),
    objLen (0),
    colLength (0),
    isWriteOnly (false),
    myMinByteToGetLength (minByteToGetLength),
    isInValid (false)
{

    if (!iterateMe.IsValid()) {
        isInValid = true;
        bytesToRequest = 0;
        myMinByteToGetLength = 0;
        return;
    }

    // process the input
    myColumn.swap (iterateMe);

    isWriteOnly = myColumn.IsWriteMode ();

    //colLength = myColumn.GetFragments().GetEndPosition(fragmentEnd);
    colLength = myColumn.GetColLength ();
    curPosInColumn = myColumn.GetFragments().GetStartPosition(fragmentStart);

    // if we essentially have an empty column, then don't get the first object
    if (colLength < minByteToGetLength)
        return;

    // get enough storage to get the object length
    uint64_t requestLen = stepSize;
    myData = myColumn.GetNewData (curPosInColumn, requestLen);
    firstInvalidByte = curPosInColumn + requestLen;
}

void Iterator::Restart(void){
    colLength = curPosInColumn; // so we do not loose the correct end
    curPosInColumn=0;
    firstInvalidByte=bytesToRequest;
    myData = myColumn.GetNewData (curPosInColumn, firstInvalidByte);
}

void Iterator :: SetFragmentRange (uint64_t start, uint64_t end) {

    assert(myColumn.GetFragments().IsValid());
    curPosInColumn = myColumn.GetFragments().GetStartPosition(start);

    // get enough storage to get the object length
    uint64_t requestLen = bytesToRequest;
    myData = myColumn.GetNewData (curPosInColumn, requestLen);
    firstInvalidByte = curPosInColumn + requestLen;
}

Iterator :: Iterator () :
    myData (NULL),
    bytesToRequest (0),
    curPosInColumn (0),
    firstInvalidByte (0),
    objLen (0),
    colLength (0),
    isWriteOnly (false),
    myMinByteToGetLength (0),
    isInValid (true) // This is imp
{
}

void Iterator :: EnsureFirstObjectSpace (uint64_t len) {

    uint64_t neededSpace = objLen;
    // For some cases we need to make sure space exists for asked number of bytes
    if (len != -1)
        neededSpace = len;

    // make sure we have the first object entirely in memory
    if (neededSpace > firstInvalidByte) {

        // now, get access to the storage
        firstInvalidByte = neededSpace;
        myData = myColumn.GetNewData (curPosInColumn, firstInvalidByte);
    }
}

Iterator :: ~Iterator () {

    // We dont need to use myColumn if it is invalid
    if (isInValid) {
        return;
    }

    // tell the column we are done
    if (curPosInColumn + objLen > colLength) {
        myColumn.Done (curPosInColumn + objLen);
    } else {
        myColumn.Done (colLength);
    }
}

void Iterator :: Done (Column &iterateMe) {

    // We dont need to use myColumn if it is invalid
    if (isInValid) {
        // even if its invalid column, give back
        iterateMe.swap (myColumn);
        return;
    }

    // tell the column we are done
    if (curPosInColumn + objLen > colLength) {
        myColumn.Done (curPosInColumn + objLen);
    } else {
        myColumn.Done (colLength);
    }

    // and return it
    iterateMe.swap (myColumn);
}

void Iterator :: Done () {

    // We dont need to use myColumn if it is invalid
    if (isInValid) {
        return;
    }

    // tell the column we are done
    if (curPosInColumn + objLen > colLength) {
        myColumn.Done (curPosInColumn + objLen);
    } else {
        myColumn.Done (colLength);
    }

    curPosInColumn = 0;
    firstInvalidByte = bytesToRequest;
    myData = myColumn.GetNewData (curPosInColumn, firstInvalidByte);
    colLength = myColumn.GetColLength ();
    isWriteOnly = myColumn.IsWriteMode ();
    assert(isWriteOnly == false);
}

void Iterator :: CreateDeepCopy (Iterator& fromMe) {

    // We should not allow to overwrite or ignore valid column
    assert (isInValid == true);

    // do shallow copy first
    //memmove (this, &fromMe, sizeof (Iterator<DataType>)); dont work

    bytesToRequest = fromMe.bytesToRequest;
    curPosInColumn = fromMe.curPosInColumn;
    firstInvalidByte = fromMe.firstInvalidByte;
    objLen = fromMe.objLen;
    colLength = fromMe.colLength;
    myMinByteToGetLength = fromMe.myMinByteToGetLength;
    isInValid = fromMe.isInValid;

    isWriteOnly = true;

    // then update relevant things deeply
    Column newCol;
    newCol.CreatePartialDeepCopy(fromMe.myColumn, curPosInColumn);
    myColumn.swap(newCol);

    // update myData pointer
    uint64_t requestLen = firstInvalidByte - curPosInColumn;
    myData = myColumn.GetNewData (curPosInColumn, requestLen);
    //firstInvalidByte += requestLen;
    firstInvalidByte = curPosInColumn + requestLen;
}

void Iterator :: swap (Iterator& swapMe) {

    myColumn.swap (swapMe.myColumn);
    SWAP_STD(myData, swapMe.myData);
    SWAP_STD(bytesToRequest, swapMe.bytesToRequest);
    SWAP_STD(curPosInColumn, swapMe.curPosInColumn);
    SWAP_STD(firstInvalidByte, swapMe.firstInvalidByte);
    SWAP_STD(objLen, swapMe.objLen);
    SWAP_STD(colLength, swapMe.colLength);
    SWAP_STD(isWriteOnly, swapMe.isWriteOnly);
    SWAP_STD(myMinByteToGetLength, swapMe.myMinByteToGetLength);
    SWAP_STD(isInValid, swapMe.isInValid);
}

void Iterator :: copy (Iterator& copyMe) {

    myColumn.copy (copyMe.myColumn);
    myData = copyMe.myData;
    bytesToRequest = copyMe.bytesToRequest;
    curPosInColumn = copyMe.curPosInColumn;
    firstInvalidByte = copyMe.firstInvalidByte;
    objLen = copyMe.objLen;
    colLength = copyMe.colLength;
    isWriteOnly = copyMe.isWriteOnly;
    myMinByteToGetLength = copyMe.myMinByteToGetLength;
    isInValid = copyMe.isInValid;
}

void Iterator :: CreateShallowCopy (Iterator& copyMe) {
    myColumn.copy (copyMe.myColumn);
    myData = copyMe.myData;
    bytesToRequest = copyMe.bytesToRequest;
    curPosInColumn = copyMe.curPosInColumn;
    firstInvalidByte = copyMe.firstInvalidByte;
    objLen = copyMe.objLen;
    colLength = copyMe.colLength;
    isWriteOnly = copyMe.isWriteOnly;
    myMinByteToGetLength = copyMe.myMinByteToGetLength;
    isInValid = copyMe.isInValid;
}


void Iterator :: ConvertFromCol(Column& realColumn) {

    if (!realColumn.IsValid())
        return;
    // process the input
    myColumn.swap (realColumn);

    isWriteOnly = myColumn.IsWriteMode ();

    // find the length of the column
    colLength = myColumn.GetColLength ();

    // if we essentially have an empty column, then don't get the first object
    if (colLength < myMinByteToGetLength)
        return;

    uint64_t requestLen = firstInvalidByte;
    if (colLength < requestLen)
        requestLen = colLength;
    // get enough storage to get the object length
    myData = myColumn.GetNewData (curPosInColumn, requestLen);
    firstInvalidByte = curPosInColumn + requestLen;
}
