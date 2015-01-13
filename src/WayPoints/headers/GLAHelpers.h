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
#ifndef _GLA_HELPER_H_
#define _GLA_HELPER_H_

#include "ID.h"
#include "Bitstring.h"
#include "Bitmap.h"
#include "Chunk.h"
#include "Bitstring.h"
#include <vector>
#include "Errors.h"

/** This file contains helper class GLAChunkCache to allow the GLAWayPoint to keep track of all the chunks produced by it.
 *  In case if a chunk is dropped and GLAWayPoint receives request for the same chunk again, it is sent from this data structure
 *  and not produced again as that particular GLAState has already been destroyed in the Finalize method.
 */

class QueryFragmentMap {

    private:
        std::vector<Bitstring> qc;

    public:
        QueryFragmentMap ();

        Bitstring GetBits (int fragmentNo);

        void Clear (int chunkNo);
        void Clear (int chunkNo, Bitstring queries);

        void ORAll (Bitstring newQ, int tillFragmentNo);
        void OROne (int fragmentNo, Bitstring newQ);

        int FindFirstSet (int _start);

        void Debugg(void);

};


inline
QueryFragmentMap::QueryFragmentMap() {
}


inline
Bitstring QueryFragmentMap::GetBits (int fragmentNo) {
    return qc[fragmentNo];
}

inline
void QueryFragmentMap::Clear (int fragmentNo) {
    qc[fragmentNo] = Bitstring(0,true);
}

inline
void QueryFragmentMap::Clear (int fragmentNo, Bitstring queries) {
    qc[fragmentNo].Difference(queries);
}

inline
void QueryFragmentMap::ORAll (Bitstring newQ, int tillFragmentNo) {
    if (tillFragmentNo>qc.size())
        qc.resize(tillFragmentNo, Bitstring(0,true));

    for (int i = 0; i < tillFragmentNo; i++) {
        qc[i].Union(newQ);
    }
}

inline
void QueryFragmentMap::OROne (int fragmentNo, Bitstring newQ) {
    newQ.Union(GetBits(fragmentNo));
    qc[fragmentNo] = newQ;
}

inline
int QueryFragmentMap::FindFirstSet (int _start) {

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
void QueryFragmentMap::Debugg(void){
    for (int i = 0; i < qc.size(); i++) {
        if (!qc[i].IsEmpty()) {
            printf("TS: %d\n", i);
        }
    }

}

#endif //  _GLA_HELPER_H_
