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


#ifndef JOIN_WP_ID_H
#define JOIN_WP_ID_H

#include "ID.h"
#include "TwoWayList.h"

// this class is a "sepcial" 10-bit identifier for join waypoints.  It is used only within the
// execution engine, and allows join waypoint identifiers to be written into the hash
// table.
class JoinWayPointID {
    unsigned int val;

public:

    void swap (JoinWayPointID &withMe) {
        SWAP_STD (val, withMe.val);
    }

    operator unsigned int () {
        return val;
    }

    void copy (JoinWayPointID &fromMe) {
        val = fromMe.val;
    }

    void operator = (const unsigned int &fromMe) {
        FATALIF (fromMe > 1023, "Putting bad val into a join waypoint ID");
        val = fromMe;
    }

    JoinWayPointID (const unsigned int &fromMe) {
        FATALIF (fromMe > 1023, "Putting bad val into a join waypoint ID");
        val = fromMe;
    }

    int IsEqual (JoinWayPointID &withMe) {
        return val == withMe.val;
    }

    JoinWayPointID () {}
    ~JoinWayPointID () {}
};

// Override global swap
inline
void swap( JoinWayPointID & a, JoinWayPointID & b ) {
    a.swap(b);
}

typedef TwoWayList <JoinWayPointID> JoinWayPointIDList;

// this maps a join waypoint ID to an actual waypoint ID
struct JoinWayPointIDMap {

    unsigned int joinWayPointID;
    WayPointID actualID;
    WayPointID diskBasedTwinID;

    JoinWayPointIDMap () {}
    ~JoinWayPointIDMap () {}

    void swap (JoinWayPointIDMap &withMe) {
        SWAP_STD (joinWayPointID, withMe.joinWayPointID);
        actualID.swap (withMe.actualID);
        diskBasedTwinID.swap (withMe.diskBasedTwinID);
    }

    void copy (JoinWayPointIDMap &fromMe) {
        joinWayPointID = fromMe.joinWayPointID;
        actualID = fromMe.actualID;
        diskBasedTwinID = fromMe.diskBasedTwinID;
    }
};

// Override global swap
inline
void swap( JoinWayPointIDMap & a, JoinWayPointIDMap & b ) {
    a.swap(b);
}

typedef TwoWayList <JoinWayPointIDMap> JoinWayPointIDEquivalences;

#endif
