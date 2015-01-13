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
#ifndef _CHUNK_ID_H
#define _CHUNK_ID_H

#include "Config.h"
#include "IDInt.h"
#include "TableScanID.h"
#include "Swap.h"
#include "TwoWayList.cc"

#include <sstream>
#include <string>

#define CHUNK_ID_ALL 833343134

/** Class to implement a slot id. The slots are used to reffer to
 * columns in a chunk and to columns on disk.
 */

class ChunkID : public IDInt {
    private:
        TableScanID scanId;
        int fragmentStart;
        int fragmentEnd;

    public:
        // we just neeed to redefine the constructor from int
        ChunkID(size_t val, TableScanID _scanId, int _fragmentStart = -1, int _fragmentEnd = -1);

        // constructor that builds an ALL chunk
        ChunkID(TableScanID& _scanId);

        // copy constructor
        ChunkID(const ChunkID& other);

        // Default constructor
        ChunkID(){ }

        // destructor
        virtual ~ChunkID() {}

        // function to test if this is an ALL chunk
        bool IsAll();

        // assignment
        ChunkID& operator = (const ChunkID& other);
        void copy (const ChunkID& other);
        bool operator<(const ChunkID& other) const;

        int GetID ();
        TableScanID GetTableScanId(void);
        int GetFragmentStart();
        int GetFragmentEnd();

        // swap for containers
        void swap (ChunkID &other);
        // the part inherited from IDInt should provide all the remaining functionality

        void getInfo(IDInfo& where);

};

typedef TwoWayList<ChunkID> ChunkIDContainer;

/** Implementation of corresponding Info class */
class ChunkInfoImp : public IDInfoImp{
    private:
        size_t id; // the numeric id so we can recreate the
        TableScanID scanId; // the table scan
        int fragmentStart;
        int fragmentEnd;

    public:
        ChunkInfoImp(size_t _id, TableScanID& _scanId, int fragmentStart = -1, int fragmentEnd = -1);

        virtual std::string getIDAsString() const OVERRIDE_SPEC;
        virtual std::string getName() const OVERRIDE_SPEC;

        virtual ~ChunkInfoImp(){ }
};

/** interface class for ChunkInfoImp */
class ChunkInfo : public IDInfo{
    public:
        // we just need a constructor
        ChunkInfo(size_t _id, TableScanID& _scan, int fragmentStart = -1, int fragmentEnd = -1);
};

/////////////////
// INLINE METHODS

inline
int ChunkID::GetID () {
    return id;
}

inline
TableScanID ChunkID::GetTableScanId(void){
    return scanId;
}

inline
int ChunkID::GetFragmentStart() {
    return fragmentStart;
}

inline
int ChunkID::GetFragmentEnd() {
    return fragmentEnd;
}

inline
void ChunkID::getInfo(IDInfo& where){
    ChunkInfo ret(id, scanId, fragmentStart, fragmentEnd);
    where.swap(ret);
}

inline
ChunkID::ChunkID(size_t val, TableScanID _scanId, int _fragStart, int _fragEnd):IDInt(val), scanId(_scanId), fragmentStart(_fragStart), fragmentEnd(_fragEnd){ }

inline
ChunkID::ChunkID(TableScanID& _scanId):IDInt(CHUNK_ID_ALL), scanId(_scanId), fragmentStart(-1), fragmentEnd(-1) { }

inline
ChunkID::ChunkID(const ChunkID& other):IDInt(other.id), scanId(other.scanId), fragmentStart(other.fragmentStart), fragmentEnd(other.fragmentEnd) {}

inline
bool ChunkID::IsAll(){ return id==CHUNK_ID_ALL; }

inline
ChunkID& ChunkID::operator = (const ChunkID& other){
    id=other.id;
    scanId=other.scanId;
    fragmentStart = other.fragmentStart;
    fragmentEnd = other.fragmentEnd;
    return (*this);
}

inline
void ChunkID::copy (const ChunkID& other){
    id=other.id;
    scanId=other.scanId;
    fragmentStart = other.fragmentStart;
    fragmentEnd = other.fragmentEnd;
}

// good for STL
inline
bool ChunkID::operator<(const ChunkID& other) const {

    if (id < other.id)
        return true;
    if (id == other.id) {
        if (scanId < other.scanId)
            return true;
        if (scanId == other.scanId) {
            if (fragmentStart < other.fragmentStart)
                return true;
            if (fragmentStart == other.fragmentStart) {
                return (fragmentEnd < other.fragmentEnd);
            }
            return false;
        }
        return false;
    }
    return false;
}

inline
void ChunkID::swap (ChunkID &other){
    SWAP_STD(id, other.id);
    scanId.swap(other.scanId);
    SWAP_STD(fragmentStart, other.fragmentStart);
    SWAP_STD(fragmentEnd, other.fragmentEnd);
}

inline
ChunkInfoImp::ChunkInfoImp(size_t _id, TableScanID& _scanId, int _fragmentStart, int _fragmentEnd ):
    id(_id), scanId(_scanId), fragmentStart(_fragmentStart), fragmentEnd(_fragmentEnd)
{ }

inline
std::string ChunkInfoImp::getIDAsString() const {
    std::stringstream s;
    std::string rez;
    if (id == ID_UNINITIALIZED){
        rez = "Uninitialized";
    } else {
        s << id ;
        s >> rez;
    }
    return rez;
}

// the name is a concatenation of Chunk and id
inline
std::string ChunkInfoImp::getName() const {
    IDInfo scanInfo;
    scanId.getInfo(scanInfo);

    std::stringstream s;
    std::string rez;
    if (id == ID_UNINITIALIZED){ {
        rez = "Uninitialized";
        assert(0);
    }
    } else {
        s << "Chunk_" << id << "_of_" << scanInfo.getName() << "_range_" << fragmentStart << "_to_" << fragmentEnd;
        s >> rez;
    }
    return rez;
}

inline
ChunkInfo::ChunkInfo(size_t _id, TableScanID& _scan, int _fragmentStart, int _fragmentEnd){
    info = new ChunkInfoImp(_id, _scan, _fragmentStart, _fragmentEnd);
}

#endif // _CHUNK_ID_H
