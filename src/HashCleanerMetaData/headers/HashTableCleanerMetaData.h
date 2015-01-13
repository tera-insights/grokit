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

#ifndef CLEANER_META_DATA_H
#define CLEANER_META_DATA_H

#include "ExecEngineData.h"
#include "HashTableSegment.h"
#include "SegmentMetaData.h"
#include "JoinWayPointID.h"
#include "HashTable.h"

// this is used by the HashTableCleaner to store all of its (complicated!) internal state, 
// related to which segemtns are being cleaned, which waypoints need to be taken out of
// which hash able segments, which queries should be purged from the hash table, who has
// acked and dropped chunks that have been created by purging the hash table, etc.
class HashTableCleanerManager {

	// this is the complete map of active join waypoints to waypoint identifiers
	TwoWayList <JoinWayPointIDMap> equivalences;

	// this is the complete list of unused join waypoint IDs
	TwoWayList <JoinWayPointID> unusedIDs;

	// this is the list of meta-data used to control cleaning for all of the hash table segs
	SegmentMetaData allSegs[NUM_SEGS];

	// this is the list of waypoints that are wounded (but not yet dead)
	TwoWayList <JoinWayPointID> wounded;

	// this is the list of waypoints that are dead
	TwoWayList <JoinWayPointID> dead;

	// this is the set of queries that have completed
	QueryExitContainer completedQueries;

	// the hash table being cleaned
	HashTable centralHashTable;

	friend class HashTableCleanerWayPointImp;

	// this sees if there are any query exits that are known to be done with and totally
	// cleaned out of the hash table.  It removes all traces of such exits from the various
	// data structures, and then returns a list of them
	void SeeIfTotallyFinished (QueryExitContainer &allDone);

protected:

	// here is how all of this works...
	//	
	// We have a global list of waypoints that are wounded and waypoints that are dead.  
	//
	// There is also a global list of completed queries (query-exit pairs) that should be removed from the hash 
	// table becuase they are no longer in use.  
	//
	// Each segment has six waypoint-oriented lists associated with it:
	//
	// 1) For wounded waypoints:
	//	a) Those wounded waypoints being cleaned
	//	b) Those wounded waypoints that have acked
	//	c) Those wounded waypoints that have dropped
	//
	// 2) For dead waypoints:
	//	a) Those dead waypoints being cleaned
	//	b) Those dead waypoints that have acked
	//	c) Those dead waypoints that have dropped
	//
	// Each segment has a list of query exit pairs that are being removed and a list that have been removed.  
	//
	// When we get an ack for a segment from a waypoint, that waypoint is moved from the "being cleaned" list
	// to the acked list.
	//
	// When we get a drop for a segment from a waypoint, that waypoint is removed from the "being cleaned" list
	// to the dropped list, and the repaired segment is deallocated (to be produced again later).
	//
	// When we get an ack for a segment, we see if there's anyone remaining to ack or drop.  If not, and we find
	// that there have been no drops, then the cleaning is done.  We then do the following:
	//
	// 1) The new version of the segment is swapped in, and 1b is emptied out.
	// 2) In addition, we check to see if any dead waypoint has been acked  by everyone.  If it has, then
	//    that waypoint is totally done, and a "QueryDone" message can be sent to the disk-based-join writer.  
	//    We also unmap the 12-bit join wapoint ID for re-use.  In the future, the planner should be
	//    notified of this, but for now we just write a message to the screen.  
	// 3) We move all of the "being cleaned" query exits to "have been cleaned".
	// 4) Next, if any completed query has now been cleaned by everyone, we write a message to the
	//    screen (in the future, someone should be notified of this as well).
	// 5) Set "isBeingCleaned" for this segment to false.
	//
	// When it comes time to get a new segment to clean, we first look to see if there is anyone who has no
	// acks remaining but has some drop.  In this case, empty out 1b, and move 1c to 1a.  Then move 2c to
	// 2a, and send him off again.
	// 
	// If there is no one like this, then look for some segment who is marked as being dirty, and clean 
	// that one.  All dead waypoints that are not in 2b are moved to 2a, and all wounded waypoints are moved
	// to 1a.
	//
	// Finally, if there is no one like this, then we will pick an arbitrary segment to clean
	// waypoint from 2b.

	// sends an ack in, and (possibly) gets back a set of completed queries to send along.  This
	// function is also the one that monitors which waypoints have acked, and whether it is safe
	// to put a newly-emptied segment into the hash table
	void GotAck (WayPointID &whichOne, int whichSegment, QueryExitContainer &completedQueries);

	// this is a lot like GotAck above, but it is called in the very special case that we obtain a cleaned
	// segment that has NO extracted chunks associated with it... in this case, we will NEVER get an
	// ack on the segment, and so we go ahead and put the newly-emptied segment back in the hash table
	// right away
	void ProcessEmptyResult (int whichSegment, QueryExitContainer &theseAreDone);
	
	// sends a drop in
	void GotDrop (WayPointID &whichOne, int whichSegment);

	// this gets a hash table segment to rebuild... returns -1 if there are none to rebuild; otherwise,
	// it returns the ID of the segment that is getting rebuilt.  The function returns a copy of the
	// actual segment to rebuild.  It also returns a list of join waypoint IDs that should be removed
	// from the segment (and sent on) and those that should be removed but not sent on.   
	// It gives back a set of completed queries to remove, as well as a
	// copy of the map from join waypoint IDs to actual waypoint IDs.  
	int GetOneToRebuild (JoinWayPointIDList &removeTheseWPsAndSend,
        	JoinWayPointIDList &removeTheseWPsAndHold, QueryExitContainer &theseQueriesAreDone,
		JoinWayPointIDEquivalences &equivalences);
	
	// this goes through the result, and extats out all of the waypoints that are in there.  If (for the
	// segment in question) there are some who have rejected previously, then we know this is a second
	// (or third, or fourth...) run and we remove any data that is not going to a reject waypoint
	void AllDone (ExtractionResult &inData, ExtractionContainer &outData);

	// this returns true iff the query ID/join waypoint combo is not already being removed from the hash table
	int IsActive(QueryID&, JoinWayPointID&);

	// this tells the hash table cleaner that the particular query has totally completed... 
	void AddCompletedQueries (QueryExitContainer &whichQueries);

	// this tells the hash table cleaner that the given segments were found to be too full
	void AreTooFull (IntList &whichSegments);

	// remember that this join waypoint is now dying
	void IsNowDying (JoinWayPointID &die);

	// tell the meta data manager that a particular waypoint has moved from wounded (meaning
	// that is was simply determined that it needed to be taken out of the hash table) to
	// dead (meaning that it will not be adding any more data to the hash table).  What happens
	// here is that the waypoint in question gets removed from the list of dying waypoints,
	// to the dead waypoint list for every individual segment
	void IsNowDead (WayPointID &whichWaypoint);

	// add a copy of the hash table to the meta data object; must be done so that the meta data
	// can replace rebuilt hash table segments
	void AddCentralHashTable (HashTable &addMe);

	// find the "real" wapoint ID corresponding to a join waypoint ID from the hash table
	void LookUpRealWaypointID (WayPointID &putItHere, JoinWayPointID &lookMeUp);

	// find the join waypoint ID for a crresponding join waypoint ID
	void LookUpJoinWaypointID (WayPointID &lookMeUp, JoinWayPointID &putItHere);
	
	// look up the writer corresponding to a particular waypoint
	void LookUpWriterForJoin (WayPointID &putItHere, JoinWayPointID &lookMeUp);
public:

	// this is called when a new join waypoint is created; it gives back a specical 10-bit
	// "join wapoint ID" that will be used to identify the wapoint in the central hash table
	unsigned int NewJoinWaypoint (WayPointID &myID, WayPointID &myDiskBasedTwinID);

	HashTableCleanerManager ();

};

#endif
