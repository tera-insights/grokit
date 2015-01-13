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


// the column we iterate over
Column myColumn;

// the current bytes we are accessed
char *myData;

// the number of bytes we get in each batch
/*const*/ int bytesToRequest;

// the current byte offset in the column
int curPosInColumn;

// the first byte we have not downloaded
int firstInvalidByte;

// tells us that the length in bytes of the current object... zero if bad
int objLen;

// length in bytes of the column
int colLength;

// Read or write mode
bool isWriteOnly;

// minimum bytes to get header length to read next object correctly
int myMinByteToGetLength;

// Do we have valid column OR column at all.
bool isInValid;

///////////////// Below are variables for checkpointing ////////////////
// Column is not needed to be checkpointed because it will be same anyway
char *c_myData;
//int c_bytesToRequest;
int c_curPosInColumn;
//int c_firstInvalidByte;
int c_objLen;
//int c_colLength;
//bool c_isWriteOnly;
//int c_myMinByteToGetLength;
//bool c_isInValid;
