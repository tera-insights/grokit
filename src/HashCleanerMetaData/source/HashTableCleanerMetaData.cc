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

#include "HashTableCleanerMetaData.h"

using namespace std;

void HashTableCleanerManager :: AddCentralHashTable (HashTable &addMe) {
	centralHashTable.Clone (addMe);
}

int HashTableCleanerManager :: IsActive (QueryID &whichQueries, JoinWayPointID &whichWayPoint) {

	// first, see if this waypoint is wounded
	for (wounded.MoveToStart (); wounded.RightLength (); wounded.Advance ()) {
		if (wounded.Current () == whichWayPoint)
			return 0;
	}

	// now, see if it is dead
	for (dead.MoveToStart (); dead.RightLength (); dead.Advance ()) {
		if (dead.Current () == whichWayPoint)
			return 0;
	}

	// now, get the equivalent regular waypoint ID
	WayPointID tempID;
	LookUpRealWaypointID (tempID, whichWayPoint);

	// subtract out all of the completed queries
	for (completedQueries.MoveToStart (); completedQueries.RightLength (); completedQueries.Advance ()) {
		if (completedQueries.Current ().exit == tempID) {
			whichQueries.Difference (completedQueries.Current ().query);
		}
	}

	// if there were no queries in there that are not completed, this one is not active
	if (whichQueries.IsEmpty ()) {
		return 0;
	}

	// if we made it this far, the slot is active!
	return 1;

}

void HashTableCleanerManager :: GotDrop (WayPointID &whichWayPoint, int whichSegment) {

	// find the equivalent join waypoint ID
	JoinWayPointID joinWayPoint;
	for (equivalences.MoveToStart (); 1; equivalences.Advance ()) {
		if (equivalences.Current ().diskBasedTwinID == whichWayPoint) {
			joinWayPoint = equivalences.Current ().joinWayPointID;
			break;
		}
	}

	// if he is wounded, move him over to dropped
	for (allSegs[whichSegment].woundedBeingCleaned.MoveToStart (); allSegs[whichSegment].woundedBeingCleaned.RightLength ();
		allSegs[whichSegment].woundedBeingCleaned.Advance ()) {
		if (allSegs[whichSegment].woundedBeingCleaned.Current () == joinWayPoint) {
			JoinWayPointID temp;
			allSegs[whichSegment].woundedBeingCleaned.Remove (temp);
			allSegs[whichSegment].woundedDropped.Insert (temp);
			break;
		}
	}

	// if he is dead, move him over to dropped
	for (allSegs[whichSegment].deadBeingCleaned.MoveToStart (); allSegs[whichSegment].deadBeingCleaned.RightLength ();
		allSegs[whichSegment].deadBeingCleaned.Advance ()) {
		if (allSegs[whichSegment].deadBeingCleaned.Current () == joinWayPoint) {
			JoinWayPointID temp;
			allSegs[whichSegment].deadBeingCleaned.Remove (temp);
			allSegs[whichSegment].deadDropped.Insert (temp);
			break;
		}
	}

	// throw out the repaired hash table segment, since we are going to have to repair it again
	HashTableSegment empty;
	empty.swap (allSegs[whichSegment].oneToPutBack);

	// and check in the segment that was dropped
	centralHashTable.CheckIn (whichSegment);
}

void HashTableCleanerManager :: ProcessEmptyResult (int whichSegment, QueryExitContainer &theseAreDone) {

	// and see if any queries are totlly finished
	SeeIfTotallyFinished (theseAreDone);

	// first note that there is no one being cleaned here
	// move all of the wounded into the "acked" bin
	allSegs[whichSegment].woundedBeingCleaned.MoveToStart ();
	allSegs[whichSegment].woundedAcked.RightLength ();
	allSegs[whichSegment].woundedBeingCleaned.SwapRights (allSegs[whichSegment].woundedAcked);

	// move all of the dead into the "acked" bin
	allSegs[whichSegment].deadBeingCleaned.MoveToStart ();
	allSegs[whichSegment].deadAcked.RightLength ();
	allSegs[whichSegment].deadBeingCleaned.SwapRights (allSegs[whichSegment].deadAcked);

	// just verify there's no one who as been dropped... this seems impossible sine by definition
	// we are here since we got an empty result... how could part of an empty result been dropped?
	allSegs[whichSegment].woundedDropped.MoveToStart ();
	allSegs[whichSegment].deadDropped.MoveToStart ();
	if (allSegs[whichSegment].woundedDropped.RightLength () || allSegs[whichSegment].deadDropped.RightLength ())
		FATAL ("How could I have had some drops off of an empty result?");

	// since everyone has acked, move the queries over
	allSegs[whichSegment].exitsCleaned.MoveToFinish ();
	allSegs[whichSegment].exitsBeingCleaned.MoveToStart ();
	allSegs[whichSegment].exitsCleaned.SwapRights (allSegs[whichSegment].exitsBeingCleaned);

	// put the newly-processed segment into the hash table
	centralHashTable.Replace (whichSegment, allSegs[whichSegment].oneToPutBack);

	// and see if some query is totally finished
	SeeIfTotallyFinished (theseAreDone);

}

void HashTableCleanerManager :: SeeIfTotallyFinished (QueryExitContainer &allDone) {

	// see if any queries have totally finished
	for (completedQueries.MoveToStart (); completedQueries.RightLength ();) {

		int count = 0;
		for (int segNum = 0; segNum < NUM_SEGS; segNum++) {
			for (allSegs[segNum].exitsCleaned.MoveToStart ();
			     allSegs[segNum].exitsCleaned.RightLength ();
			     allSegs[segNum].exitsCleaned.Advance ()) {
				if (allSegs[segNum].exitsCleaned.Current () == completedQueries.Current ()) {
					count++;
					break;
				}
			}
		}

		// this one has finished, so remove it
		if (count == NUM_SEGS) {
			QueryExit temp;
			completedQueries.Remove (temp);
		} else {
			completedQueries.Advance ();
			continue;
		}

		// loop through and kill each copy
		for (int segNum = 0; segNum < NUM_SEGS; segNum++) {
			for (allSegs[segNum].exitsCleaned.MoveToStart ();
			     allSegs[segNum].exitsCleaned.RightLength ();) {
				if (allSegs[segNum].exitsCleaned.Current () == completedQueries.Current ()) {
					QueryExit temp;
					allSegs[segNum].exitsCleaned.Remove (temp);
				} else {
					allSegs[segNum].exitsCleaned.Advance ();
				}
			}
		}
	}

	// empty out the result
	QueryExitContainer emptyAgain;
	emptyAgain.swap (allDone);

	// see if any waypoints have totally finished
	for (dead.MoveToStart (); dead.RightLength ();) {

		int count = 0;
		for (int segNum = 0; segNum < NUM_SEGS; segNum++) {
			for (allSegs[segNum].deadAcked.MoveToStart ();
			     allSegs[segNum].deadAcked.RightLength ();
			     allSegs[segNum].deadAcked.Advance ()) {
				if (allSegs[segNum].deadAcked.Current () == dead.Current ()) {
					count++;
					break;
				}
			}
		}

		// this one has finished, so remove it
		if (count != NUM_SEGS) {

			WayPointID temp;
			LookUpRealWaypointID (temp, dead.Current ());
			cout << "Cleaned " << count << " out of " << NUM_SEGS << " for dead waypoint ";
			temp.Print ();
			cout << "\n";
			dead.Advance ();
			continue;
		}

		// loop through and kill each copy of the current guy in "dead"
		for (int segNum = 0; segNum < NUM_SEGS; segNum++) {
			for (allSegs[segNum].deadAcked.MoveToStart ();
			     allSegs[segNum].deadAcked.RightLength ();) {
				if (allSegs[segNum].deadAcked.Current () == dead.Current ()) {
					JoinWayPointID temp;
					allSegs[segNum].deadAcked.Remove (temp);
				} else {
					allSegs[segNum].deadAcked.Advance ();
				}
			}
		}

		// remove the current guy from the dead ones
		JoinWayPointID temp;
		dead.Remove (temp);

		// find its "real" corresponding waypoint ID
		WayPointID realOne;
		LookUpWriterForJoin (realOne, temp);

		// remember the completed query exit
		QueryExit deadOne;
		deadOne.exit = realOne;
		deadOne.query.Empty ();
		allDone.Insert (deadOne);

	}

}

void HashTableCleanerManager :: GotAck (WayPointID &whichWaypoint, int whichSegment, QueryExitContainer &allDone) {

	JoinWayPointID joinWayPoint;
	for (equivalences.MoveToStart (); 1; equivalences.Advance ()) {
		if (equivalences.Current ().diskBasedTwinID == whichWaypoint) {
			joinWayPoint = equivalences.Current ().joinWayPointID;
			break;
		}
	}

	// if he is wounded, move him over to wounded + acked
	for (allSegs[whichSegment].woundedBeingCleaned.MoveToStart (); allSegs[whichSegment].woundedBeingCleaned.RightLength ();
		allSegs[whichSegment].woundedBeingCleaned.Advance ()) {
		if (allSegs[whichSegment].woundedBeingCleaned.Current () == joinWayPoint) {
			JoinWayPointID temp;
			allSegs[whichSegment].woundedBeingCleaned.Remove (temp);
			allSegs[whichSegment].woundedAcked.Insert (temp);
			break;
		}
	}

	// if he is dead, move him over to dead + acked
	for (allSegs[whichSegment].deadBeingCleaned.MoveToStart (); allSegs[whichSegment].deadBeingCleaned.RightLength ();
		allSegs[whichSegment].deadBeingCleaned.Advance ()) {
		if (allSegs[whichSegment].deadBeingCleaned.Current () == joinWayPoint) {
			JoinWayPointID temp;
			allSegs[whichSegment].deadBeingCleaned.Remove (temp);
			allSegs[whichSegment].deadAcked.Insert (temp);
			break;
		}
	}

	// if it is not the case that everyone has acked, then just exit
	allSegs[whichSegment].deadBeingCleaned.MoveToStart ();
	allSegs[whichSegment].woundedBeingCleaned.MoveToStart ();
	allSegs[whichSegment].deadDropped.MoveToStart ();
	allSegs[whichSegment].woundedDropped.MoveToStart ();

	if (allSegs[whichSegment].deadBeingCleaned.RightLength () ||
	    allSegs[whichSegment].woundedBeingCleaned.RightLength () ||
	    allSegs[whichSegment].deadDropped.RightLength () ||
	    allSegs[whichSegment].woundedDropped.RightLength ()) {
		return;
	}

	// empty out the wounded acked
	JoinWayPointIDList empty;
	empty.swap (allSegs[whichSegment].woundedAcked);

	// since everyone has acked, move the queries over
	allSegs[whichSegment].exitsCleaned.MoveToFinish ();
	allSegs[whichSegment].exitsBeingCleaned.MoveToStart ();
	allSegs[whichSegment].exitsCleaned.SwapRights (allSegs[whichSegment].exitsBeingCleaned);

	// put the newly-processed segment into the hash table
	centralHashTable.Replace (whichSegment, allSegs[whichSegment].oneToPutBack);

	// and see if some query is totally finished
	SeeIfTotallyFinished (allDone);
}

void HashTableCleanerManager :: IsNowDying (JoinWayPointID &die) {

	// super easy: just add to the list of dying waypoints
	wounded.Insert (die);
}

unsigned int HashTableCleanerManager ::  NewJoinWaypoint (WayPointID &myID, WayPointID &myDiskBasedTwinID) {

	// get a new ID
	JoinWayPointID temp;
	unusedIDs.MoveToStart ();

	if (!unusedIDs.RightLength ())
		FATAL ("Ran out of join waypoint IDs!\n");

	unusedIDs.Remove (temp);
	unsigned int returnVal = temp;
	cout << "Returning " << returnVal << " as the new JWPID for ";
	myID.Print ();
	cout << "\n";

	// and remember the mapping
	JoinWayPointIDMap record;
	record.joinWayPointID = returnVal;
	record.actualID = myID;
	record.diskBasedTwinID = myDiskBasedTwinID;
	equivalences.Insert (record);

	// outta here!
	return returnVal;

}

void HashTableCleanerManager :: AddCompletedQueries (QueryExitContainer &addMe) {
	completedQueries.MoveToFinish ();
	addMe.MoveToStart ();
	completedQueries.SwapRights (addMe);
	cerr << "\n";
	for (completedQueries.MoveToStart (); completedQueries.RightLength (); completedQueries.Advance ()) {
		completedQueries.Current ().Print ();
		cerr << "\n";
	}
}

void HashTableCleanerManager :: LookUpWriterForJoin (WayPointID &putItHere, JoinWayPointID &lookMeUp) {

	for (equivalences.MoveToStart (); 1; equivalences.Advance ()) {
		if (equivalences.Current ().joinWayPointID == lookMeUp) {
			putItHere = equivalences.Current ().diskBasedTwinID;
			break;
		}
	}
}

void HashTableCleanerManager :: LookUpJoinWaypointID (WayPointID &lookMeUp, JoinWayPointID &putItHere) {

	for (equivalences.MoveToStart (); 1; equivalences.Advance ()) {
		if (equivalences.Current ().actualID == lookMeUp) {
			putItHere = equivalences.Current ().joinWayPointID;
			break;
		}
	}
}

void HashTableCleanerManager :: LookUpRealWaypointID (WayPointID &putItHere, JoinWayPointID &lookMeUp) {

	for (equivalences.MoveToStart (); 1; equivalences.Advance ()) {
		if (equivalences.Current ().joinWayPointID == lookMeUp) {
			putItHere = equivalences.Current ().actualID;
			break;
		}
	}
}

void HashTableCleanerManager :: IsNowDead (WayPointID &whichWaypoint) {

	// first, find the equivalent join waypoint ID
	JoinWayPointID whichJoinWaypoint;
	LookUpJoinWaypointID (whichWaypoint, whichJoinWaypoint);

	// now, remove it from the set of wounded waypoints
	for (wounded.MoveToStart (); wounded.RightLength (); wounded.Advance ()) {
		if (wounded.Current () == whichJoinWaypoint) {
			JoinWayPointID foo;
			wounded.Remove (foo);
			break;
		}
	}

	cout << "Is now dead: ";
	whichWaypoint.Print ();
	cout << "\n";
	dead.Insert (whichJoinWaypoint);
}

void HashTableCleanerManager :: AllDone (ExtractionResult &inData, ExtractionContainer &outData) {

	// first, just remember the actual segment that we received
	int whichSegToProcess = inData.get_whichSegment ();
	allSegs[whichSegToProcess].oneToPutBack.swap (inData.get_newSegment ());

	// an return the result!
	outData.swap (inData.get_result ());
}


int HashTableCleanerManager :: GetOneToRebuild (JoinWayPointIDList &removeTheseWPsAndSend,
	JoinWayPointIDList &removeTheseWPsAndHold, QueryExitContainer &theseQueriesAreDone, JoinWayPointIDEquivalences &equivs) {

	// note that we have no idea which segment to process
	int whichSegToProcess = -1;

	// first, look for a hash table segment with a rejection
	for (int whichSegment = 0; whichSegment < NUM_SEGS; whichSegment++) {
		allSegs[whichSegment].deadBeingCleaned.MoveToStart ();
		allSegs[whichSegment].woundedBeingCleaned.MoveToStart ();
		allSegs[whichSegment].deadDropped.MoveToStart ();
		allSegs[whichSegment].woundedDropped.MoveToStart ();
	}

	// if a segment has no waypoints being cleaned but it has a drop, then it can/should be re-processed
	for (int whichSegment = 0; whichSegment < NUM_SEGS; whichSegment++) {
		if (!allSegs[whichSegment].deadBeingCleaned.RightLength () &&
		    !allSegs[whichSegment].woundedBeingCleaned.RightLength () &&
		    (allSegs[whichSegment].deadDropped.RightLength () ||
		    allSegs[whichSegment].woundedDropped.RightLength ())) {

			// note that this is the one to process
			whichSegToProcess = whichSegment;

			// copy the dropped ones back over to the ones being cleaned
			allSegs[whichSegment].deadDropped.swap (allSegs[whichSegment].deadBeingCleaned);
			allSegs[whichSegment].woundedDropped.swap (allSegs[whichSegment].woundedBeingCleaned);

			// forget the acked, wounded waypoints
			JoinWayPointIDList empty;
			empty.swap (allSegs[whichSegment].woundedAcked);

			break;
		}

	}

	// if we didn't find a segment that needs to be re-processed, then check another one out
	if (whichSegToProcess == -1) {

		// find all of the segments to process
		int whichSegs[NUM_SEGS];
		// Modified by Alin
		// Problem: The cleaner does not use all the CPUs
		// Reason: Only few buckets reach the overfull boundary at a time
		//   while Cleaner cleans, no waypoint can fill more bukets.
		//   this crates ugly oscilations
		// Solution: Clean almost full buckets as well
		// If the second argument to CheckOverFull is deleted, Chris' original
		// behavior is obtained.
		int gotOne = centralHashTable.CheckOverFull (whichSegs, CLEAN_FILL_RATE);
		for (int i = 0; i < NUM_SEGS; i++) {

			// if we have received a cleaning request, and we are not currently cleaning
			if (whichSegs[i] && (allSegs[i].deadBeingCleaned.RightLength () ||
			    allSegs[i].woundedBeingCleaned.RightLength ())) {
				whichSegs[i] = 0;
				gotOne--;
			}
		}

		if (gotOne) {

			// choose one of them to process
			int whichOne = lrand48 () % gotOne, curOne = 0;
			while (1) {
				if (whichSegs[curOne] == 1) {
					if (whichOne == 0) {
						whichSegToProcess = curOne;
						break;
					} else {
						whichOne--;
					}
				}
				curOne = (curOne + 1) % NUM_SEGS;
			}

			// set up the lists of the waypoints we want to clean
			allSegs[whichSegToProcess].deadBeingCleaned.copy (dead);
			allSegs[whichSegToProcess].woundedBeingCleaned.copy (wounded);

			// don't clean out those dead that have already been acked
			allSegs[whichSegToProcess].deadBeingCleaned.Subtract (allSegs[whichSegToProcess].deadAcked);
		}
	}

	// find one segment with a dead waypoint that has not been removed
	if (whichSegToProcess == -1) {
		for (int whichSegment = 0; whichSegment < NUM_SEGS && whichSegToProcess == -1; whichSegment++) {

			// make sure this one is not being processed
			if (allSegs[whichSegment].deadBeingCleaned.RightLength () ||
			    allSegs[whichSegment].woundedBeingCleaned.RightLength ()) {
				continue;
			}

			// see if this guy has not acked any of the dead waypoints
			for (dead.MoveToStart (); dead.RightLength (); dead.Advance ()) {

				// see if we have not acked the dead waypoint
				int gotOne = 0;
				for (allSegs[whichSegment].deadAcked.MoveToStart ();
				     allSegs[whichSegment].deadAcked.RightLength ();
				     allSegs[whichSegment].deadAcked.Advance ()) {
					if (allSegs[whichSegment].deadAcked.Current () == dead.Current ())
						gotOne = 1;
				}

				// if we have not acked it, then we will process this guy
				if (!gotOne) {
					whichSegToProcess = whichSegment;
					JoinWayPointID temp = dead.Current ();
					allSegs[whichSegment].deadBeingCleaned.Insert (temp);
				}
			}

		}
	}

	// if there are no segs to process, then just get outta here
	if (whichSegToProcess == -1)
		return -1;

	// set up the exits being cleaned list for this segment
	allSegs[whichSegToProcess].exitsBeingCleaned.copy (completedQueries);
	allSegs[whichSegToProcess].exitsBeingCleaned.Subtract (allSegs[whichSegToProcess].exitsCleaned);

	// set up the waypoints we are supposed to remove and send... this is everyone who dropped
	removeTheseWPsAndSend.copy (allSegs[whichSegToProcess].woundedBeingCleaned);
	JoinWayPointIDList temp;
	temp.copy (allSegs[whichSegToProcess].deadBeingCleaned);
	temp.MoveToStart ();
	removeTheseWPsAndSend.MoveToFinish ();
	removeTheseWPsAndSend.SwapRights (temp);

	// now set up the ones we are supposed to remove and hold
	removeTheseWPsAndHold.copy (allSegs[whichSegToProcess].deadAcked);
	temp.copy (allSegs[whichSegToProcess].woundedAcked);
	temp.MoveToStart ();
	removeTheseWPsAndHold.MoveToFinish ();
	removeTheseWPsAndHold.SwapRights (temp);

	// take care of setting up the queries to remove
	theseQueriesAreDone.copy (completedQueries);
	theseQueriesAreDone.Subtract (allSegs[whichSegToProcess].exitsCleaned);
	allSegs[whichSegToProcess].exitsBeingCleaned.copy (theseQueriesAreDone);

	// set up the list of equivs to return
	equivs.copy (equivalences);

	// and get outta here
	return whichSegToProcess;
}

HashTableCleanerManager :: HashTableCleanerManager () {

	// since we use 10 bits for the JoinWayPointID, there are 1024 ids available
	for (int i = 0; i < 1024; i++) {
		JoinWayPointID newID = i;
		unusedIDs.Insert (newID);
	}
}

