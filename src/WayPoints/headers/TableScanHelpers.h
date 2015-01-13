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
#ifndef _TABLE_SCAN_HELPER_H_
#define _TABLE_SCAN_HELPER_H_

#include "ID.h"
#include "Bitstring.h"
#include "Bitmap.h"
#include "Chunk.h"
#include "Bitstring.h"
#include <map>
#include "Errors.h"

/** This file contains helper classes to allow the scanners
  maintainance of the datastructures needed to more cleanly deal
  with chunk/data production. The two main users of these classes is
  the single and tile scanners.

*/

/** Class to deal with translations between QueryExits and bitstrings.

Rationale: bitstrings are much easier to manipulate and are
compact but the rest of the system needs QueryExits.

The class maintains an internal mapping between bitstrings and
QueryExits. These bitstrings are not manipulated and maintained by
the QueryManager (that just happens to use bistrings as well).

*/

class QEToBitstring {
    private:
        // variable to keep track of all the queries we work on
        Bitstring queryExits;

        // map between the bitsring and the QueryExit
        std::map < Bitstring, QueryExit > bitstringToQueryExitMap;

        // map between the QueryExit and the bitstring
        std::map < QueryExit, Bitstring > queryExitToBitstringMap;

        // this function translates between bitstrings representing QueryExits into QueryIDSet (set of queries)
        QueryIDSet queryExitToQueryIDSet(Bitstring what);

    public:
        // constructor. almost nothing to do
        QEToBitstring(){ queryExits.Empty(); }

        // Auxiliary translation functions
        void bitstringToQueryExitContainer(Bitstring what, QueryExitContainer& where);
        void bitstringToQueryExitContaiener(Bitstring what, QueryExitContainer& where) {
            bitstringToQueryExitContainer(what, where);
        }

        // this function allocates a new bitstirng if it cannot find one allocated if create = true
        Bitstring queryExitToBitstring(QueryExitContainer& what, bool create = false);
        // delete some queryExits from the system
        void deleteQueryExits(QueryExitContainer& what);

};


/** Class to keep track of columns that are needed for each query.  We
  keep track of both the logical columns as well as the mapping to
  physical columns. That means that the low-level file scanner is
  now completely dissociated from logical knowledge.

  This class needs to be instantiated once per relation tracked.

*/

class ColumnManager {
    private:
        //map between queries and the columns they need
        QueryExitToSlotsMap queryColumnsMap;

        // global map of chunk slots into disk columns. This is not a per
        // query map since chunk slot alocation is the same for the same
        // attribute
        SlotToSlotMap slotsToColumnsMap;

        // Same as above, but for writing.
        SlotToSlotMap storeSlotsToColumnsMap;

    public:
        // This function allows changing the mapping of QE to columns same
        // for physical to logical columns information sent is the extra
        // column info on QE, new logical to physical column mappings and
        // deletedQE
        void ChangeMapping(QueryExitToSlotsMap& _queryColumnsMap,
                SlotToSlotMap& _columnsToSlotsMap, SlotToSlotMap& _storeColumnsToSlotsMap,
                QueryExitContainer& qExitsDone);

        //compute the union of all the columnscolumns used in the active queries;
        // where indicates where to place the result
        // the result is the logicalSlot X Physical column pair
        // Note: the function does both the union and translation to physical columns
        void UnionColumns(QueryExitContainer& queries, SlotPairContainer& where);

        // functin to compute placement information for writing chunchs
        // essentially the info is pulled from the queryColumnsMap
        void GetColsToWrite(SlotPairContainer& where);
};


class QueryChunkMap {

    private:
        std::vector<Bitstring> qc;

    public:
        QueryChunkMap (int numChunks);

        Bitstring GetBits (int chunkNo);

        void Clear (int chunkNo);

        void ORAll (Bitstring newQ);
        void OROne (int chunkNo, Bitstring newQ);
        void DiffAll (Bitstring query);
        void DiffOne (int chunkNo, Bitstring queries);

        int FindFirstSet (int _start);

        void Debugg(void);

};


inline
QueryChunkMap::QueryChunkMap(int numChunks) {
    qc.resize(numChunks, Bitstring(0,true));
}


inline
Bitstring QueryChunkMap::GetBits (int chunkNo) {
    return qc[chunkNo];
}

inline
void QueryChunkMap::Clear (int chunkNo) {
    qc[chunkNo] = Bitstring(0,true);
}

inline
void QueryChunkMap::ORAll (Bitstring newQ) {
    for (int i = 0; i < qc.size(); i++) {
        qc[i].Union(newQ);
    }
}

inline
void QueryChunkMap::OROne (int chunkNo, Bitstring newQ) {
    newQ.Union(GetBits(chunkNo));
    qc[chunkNo] = newQ;
}

inline
void QueryChunkMap :: DiffAll( Bitstring query ) {
    for( int i = 0; i < qc.size(); ++i ) {
        qc[i].Difference(query);
    }
}

inline
void QueryChunkMap::DiffOne( int chunkNo, Bitstring queries ) {
    qc[chunkNo].Difference(queries);
}

inline
int QueryChunkMap::FindFirstSet (int _start) {

    for (int i = _start; i < qc.size(); i++) {
        if (!qc[i].IsEmpty()) {
            return i;
        }
    }

    for (int i = 0; i < _start; i++) {
        if (!qc[i].IsEmpty()) {
            return i;
        }
    }

    return -1;
}

inline
void QueryChunkMap::Debugg(void){
    for (int i = 0; i < qc.size(); i++) {
        if (!qc[i].IsEmpty()) {
            printf("TS: %d\n", i);
        }
    }

}

#endif //  _TABLE_SCAN_HELPER_H_
