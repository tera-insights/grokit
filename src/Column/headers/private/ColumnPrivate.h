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

friend class Iterator;
friend class ColumnReadWrite;

private:

    // protects the reference counter
    DistributedCounter* refCount; // reference counter

    // the actual column storage
    MMappedStorage *myData;

    // Fragmants class to store metadata about chunk fragments
    Fragments fragments;

    Destroyer columnDestroyer;

protected:

    // get the number of bytes stored in the column
    uint64_t GetColLength ();

    // tell the column we are done iterating, and that it should be truncated at numBytes
    void Done (uint64_t numBytes);

    // tell the column that we want to detach from any shallow copies (if they exist) by
    // making a deep copy
    //void Detach ();

    // ask the column for more data; this call goes straight through to the storage
    char *GetNewData (uint64_t posToStartFrom, uint64_t &numBytesRequested);

