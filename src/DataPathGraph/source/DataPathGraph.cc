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

#include "DataPathGraph.h"
#include "Swap.h"
#include <iostream>

void DataPathGraph :: swap (DataPathGraph &withMe) {

    SWAP_memmove(DataPathGraph, withMe.myGraph);
    /*
       char temp[sizeof (DataPathGraph)];
       memmove (temp, this, sizeof (DataPathGraph));
       memmove (this, &withMe, sizeof (DataPathGraph));
       memmove (&withMe, temp, sizeof (DataPathGraph));
       */

}

void DataPathGraph :: FindUpstreamWaypoints (WayPointID &currentPos, QueryExit &dest, WayPointIDContainer &nextOnes) {

    // empty out the answer set
    WayPointIDContainer empty;
    empty.swap (nextOnes);

    // try to get the node
    if (!myGraph.IsThere (currentPos))
        FATAL ("You can't find the way out of a node that does not exist!");

    // get all of the data for that node
    NodeInfo &data = myGraph.CurrentData ();

    // loop through all of the in links
    for (data.linksTo.MoveToStart (); !data.linksTo.AtEnd (); data.linksTo.Advance ()) {

        // get the set of query exits that go to us
        QueryExitContainer &allLinks = data.linksTo.CurrentData ();

        // see if any of those links matches
        for (allLinks.MoveToStart (); allLinks.RightLength (); allLinks.Advance ()) {

            // it does, so remember the waypoint ID!
            if (allLinks.Current ().IsEqual (dest)) {
                WayPointID temp;
                temp = data.linksTo.CurrentKey ();
                nextOnes.Insert (temp);
                break;
            }
        }
    }
}

void DataPathGraph :: FindAllRoutings (WayPointID &currentPos, QueryExitContainer &dests,
        InefficientMap <WayPointID, QueryExitContainer> &allSubsets) {

    // first, empty out the return val
    InefficientMap <WayPointID, QueryExitContainer> temp;
    temp.swap (allSubsets);

    // try to get the node
    if (!myGraph.IsThere (currentPos))
        FATAL ("You can't find the terminal links into a node that does not exist!");

    // get all of the data for that node
    NodeInfo &data = myGraph.CurrentData ();

    // loop through all of the destinations
    for (data.linksFrom.MoveToStart (); !data.linksFrom.AtEnd (); data.linksFrom.Advance ()) {

        // get the waypoint at the other end of that link
        WayPointID otherEnd = data.linksFrom.CurrentKey ();

        // get the set of query exits that go down the link
        QueryExitContainer &allLinks = data.linksFrom.CurrentData ();

        // this will be the set of query exits in common with the query set
        QueryExitContainer queryExits;

        // now join the two sets
        for (allLinks.MoveToStart (); allLinks.RightLength (); allLinks.Advance ()) {
            for (dests.MoveToStart (); dests.RightLength (); dests.Advance ()) {
                if (allLinks.Current (). IsEqual (dests.Current ())) {
                    QueryExit temp;
                    temp.copy (allLinks.Current ());
                    queryExits.Insert (temp);
                }
            }
        }

        // if there is anything in the join, remember it
        if (queryExits.RightLength ()) {
            allSubsets.Insert (otherEnd, queryExits);
        }
    }
}

void DataPathGraph :: AddNode (WayPointID &myID) {
    if (myGraph.IsThere (myID)) {
        FATAL ("You can't add the same node to the graph twice!\n");
    } else {
        WayPointID tempID = myID;
        NodeInfo tempInfo;
        myGraph.Insert (tempID, tempInfo);
    }
}

void DataPathGraph :: AddLink (WayPointID &fromIn, WayPointID &toIn, QueryExit &pairIn) {

    // make copies of the input objects so we do not stomp on them
    WayPointID from = fromIn, to = toIn;
    QueryExit pair = pairIn;

    if (!myGraph.IsThere (from))
        FATAL ("You can't add a link from a node that does not exist!");

    // add the link to the second node
    QueryExit tempPair;
    tempPair = pair;

    // if the second node is not already linked to, then add the link
    if (myGraph.CurrentData ().linksFrom.IsThere (to)) {
        myGraph.CurrentData ().linksFrom.CurrentData ().Insert (tempPair);

        // if the second node is NOT linked to, then need to add the node as well
    } else {
        WayPointID tempTo;
        tempTo = to;
        QueryExitContainer tempContainer;
        tempContainer.Insert (tempPair);
        myGraph.CurrentData ().linksFrom.Insert (tempTo, tempContainer);
    }

    // now, try to add the link into the to node
    if (!myGraph.IsThere (to))
        FATAL ("You can't add a link from a node that does not exist!");

    // if the to node is not already linked into, then add the link
    if (myGraph.CurrentData ().linksTo.IsThere (from)) {
        myGraph.CurrentData ().linksTo.CurrentData ().Insert (pair);

        // if the second node is NOT linked to, then need to add the node as well
    } else {
        QueryExitContainer tempContainer;
        tempContainer.Insert (pair);
        myGraph.CurrentData ().linksTo.Insert (from, tempContainer);
    }
}

