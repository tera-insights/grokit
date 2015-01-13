
// used to construct a lookup table to see what we need to do with each hash entry
struct WayPointInformation {
    int isDying;
    int index;
    QueryID killThese;
};

#define DYING_AND_HOLD 1
#define DYING_AND_SEND 2
#define ALIVE 0

#include "ColumnIterator.h"
#include "MMappedStorage.h"
extern "C"
int CleanerWorkFunc_<//>M4_WPName (WorkDescription &workDescription, ExecEngineData &result) {

    // M4_LHS_Att = M4_LHS_Attr
    // M4_RHS_Att = M4_RHS_Attr
    // M4_All_Querie = M4_All_Queries

    int counter = 0;

    // this is the area where all of the records are serialized to;
    // 10K bytes are initially used for this
    void *serializeHere = (void *) malloc (10000);

    // get the work description
    HashCleanerWorkDescription myWork;
    myWork.swap (workDescription);

    // now exract the list of dying waypoints... these are ones to create chunks from
    myWork.get_dyingWayPointsToSend ().MoveToStart ();
    int numDyingWaypoints = myWork.get_dyingWayPointsToSend ().RightLength ();

    // this 1024 is a magic number, since we can have 1024 join waypoints (10 bits are devoted to the join waypoint ID)
    WayPointInformation *wayPointInfo = new WayPointInformation[1024];
    for (int i = 0; i < 1024; i++) {
        wayPointInfo[i].isDying = ALIVE;
        wayPointInfo[i].index = -1;
        wayPointInfo[i].killThese.Empty ();
    }
    // mark all of the dying waypoints that need to be sent on
    for (int i = 0; myWork.get_dyingWayPointsToSend ().RightLength (); myWork.get_dyingWayPointsToSend ().Advance (), i++) {
        wayPointInfo[myWork.get_dyingWayPointsToSend ().Current ()].isDying = DYING_AND_SEND;
        wayPointInfo[myWork.get_dyingWayPointsToSend ().Current ()].index = i;
    }

    // mark all of the dying waypoints that need to be held
    myWork.get_dyingWayPointsToHold ().MoveToStart ();
    for (; myWork.get_dyingWayPointsToHold ().RightLength (); myWork.get_dyingWayPointsToHold ().Advance ()) {
        wayPointInfo[myWork.get_dyingWayPointsToHold ().Current ()].isDying = DYING_AND_HOLD;
    }

    // now, go through and remember all of the queries that we need to hold back
    myWork.get_theseQueriesAreDone ().MoveToStart ();
    for (; myWork.get_theseQueriesAreDone ().RightLength (); myWork.get_theseQueriesAreDone ().Advance ()) {

        // find the equivalent join waypoint id
        for (myWork.get_equivalences ().MoveToStart (); ; myWork.get_equivalences ().Advance ()) {
            if (myWork.get_theseQueriesAreDone ().Current ().exit == myWork.get_equivalences ().Current ().actualID) {
                int index = myWork.get_equivalences ().Current ().joinWayPointID;
                wayPointInfo[index].killThese.Union (myWork.get_theseQueriesAreDone ().Current ().query);
                break;
            }
            if (!(myWork.get_equivalences ().RightLength ())) {
                cout << "Cound not find a match for ";
                myWork.get_theseQueriesAreDone ().Current ().Print ();
                cout << "\n";
                cout << "I found ";
                for (myWork.get_equivalences ().MoveToStart (); ; myWork.get_equivalences ().Advance ()) {
                    cout << "\n";
                    myWork.get_equivalences ().Current ().actualID.Print ();
                    cout << "\n";
                }
            }
        }
    }

    // check out the hash table segment we are processing
    HashTableSegment mySegment;
    int whichSegment = myWork.get_whichSegment ();
    int theseAreOK[NUM_SEGS];
    for (int i = 0; i < NUM_SEGS; i++) {
        theseAreOK[i] = 0;
    }
    theseAreOK[whichSegment] = 1;
    myWork.get_centralHashTable ().CheckOutOne (theseAreOK, mySegment);

    // now set up the various attribute columns...

    // LHS IS FIRST
    // first is the hash column
    Column hashColumnLHS[numDyingWaypoints];
    ColumnIterator <HT_INDEX_TYPE> hashColumnIterLHS[numDyingWaypoints];

    // now is the bitmap column
    //Column bitmapColumnLHS[numDyingWaypoints];
    BStringIterator bitmapColumnIterLHS[numDyingWaypoints];
    int bitmapColumnLHSIsUsed[numDyingWaypoints];

    // now are all of the LHS data columns... there is one of these for each LHS join column
    // IN THE ENTIRE SYSTEM
<//><//>m4_foreach(</_A_/>,m4_quote(reval(</m4_args/>M4_LHS_Attr)),</dnl
<//><//><//>M4_IFVALID_ATT(_A_, </dnl
    Column _A_<//>_Column_LHS[numDyingWaypoints];
    ColumnIterator <M4_ATT_TYPE(_A_)> _A_<//>_ColumnIter_LHS[numDyingWaypoints];
    int _A_<//>_Column_LHSIsUsed[numDyingWaypoints];
<//><//><//>/>)<//>dnl
<//><//>/>)<//>dnl

    // now, actually set up all of these columns
    for (int i = 0; i < numDyingWaypoints; i++) {

        // set up the hash column
        MMappedStorage hashStore;
        Column hash (hashStore);
        ColumnIterator <HT_INDEX_TYPE> hashIter (hash);
        hashIter.swap (hashColumnIterLHS[i]);

        // now is the bitmap... set it up so that it contains ALL of the queries
        // that could possibly be removed from the hash table
        MMappedStorage bitmapStore;
        Bitstring tempBits(M4_All_Queries, true);
        Column bitmap (bitmapStore);
        BStringIterator bitmapIter (bitmap, tempBits);
        bitmapIter.swap (bitmapColumnIterLHS[i]);
        bitmapColumnLHSIsUsed[i] = 0;

        // now all of the LHS data columns
<//><//>m4_foreach(</_A_/>,m4_quote(reval(</m4_args/>M4_LHS_Attr)),</dnl
<//><//><//>M4_IFVALID_ATT(_A_, </dnl
        MMappedStorage _A_<//>_Column_Store;
        Column _A_<//>_Col (_A_<//>_Column_Store);
        ColumnIterator <M4_ATT_TYPE(_A_)> _A_<//>_ColumnIter (_A_<//>_Col);
        _A_<//>_ColumnIter.swap (_A_<//>_ColumnIter_LHS[i]);
        _A_<//>_Column_LHSIsUsed[i] = 0;
<//><//><//>/>)<//>dnl
<//><//>/>)<//>dnl

    }

    // RHS IS NEXT
    // first is the hash column
    Column hashColumnRHS[numDyingWaypoints];
    ColumnIterator <HT_INDEX_TYPE> hashColumnIterRHS[numDyingWaypoints];

    // now is the bitmap column
    Column bitmapColumnRHS[numDyingWaypoints];
    BStringIterator bitmapColumnIterRHS[numDyingWaypoints];
    int bitmapColumnRHSIsUsed[numDyingWaypoints];

    // now are all of the RHS data columns... there is one of these for each RHS join column
    // IN THE ENTIRE SYSTEM
<//><//>m4_foreach(</_A_/>,m4_quote(reval(</m4_args/>M4_RHS_Attr)),</dnl
<//><//><//>M4_IFVALID_ATT(_A_, </dnl
    Column _A_<//>_Column_RHS[numDyingWaypoints];
    ColumnIterator <M4_ATT_TYPE(_A_)> _A_<//>_ColumnIter_RHS[numDyingWaypoints];
    int _A_<//>_Column_RHSIsUsed[numDyingWaypoints];
<//><//><//>/>)<//>dnl
<//><//>/>)<//>dnl

    // now, actually set up all of these columns
    for (int i = 0; i < numDyingWaypoints; i++) {

        // set up the hash column
        MMappedStorage hashStore;
        Column hash (hashStore);
        ColumnIterator <HT_INDEX_TYPE> hashIter (hash);
        hashIter.swap (hashColumnIterRHS[i]);

        // now is the bitmap
        MMappedStorage bitmapStore;
        Column bitmap (bitmapStore);
        Bitstring tempBits(M4_All_Queries,true);
        BStringIterator bitmapIter (bitmap, tempBits);
        bitmapIter.swap (bitmapColumnIterRHS[i]);
        bitmapColumnRHSIsUsed[i] = 0;

        // now all of the RHS data columns
<//><//>m4_foreach(</_A_/>,m4_quote(reval(</m4_args/>M4_RHS_Attr)),</dnl
<//><//><//>M4_IFVALID_ATT(_A_, </dnl
        MMappedStorage _A_<//>_Column_Store;
        Column _A_<//>_Col (_A_<//>_Column_Store);
        ColumnIterator <M4_ATT_TYPE(_A_)> _A_<//>_ColumnIter (_A_<//>_Col);
        _A_<//>_ColumnIter.swap (_A_<//>_ColumnIter_RHS[i]);
        _A_<//>_Column_RHSIsUsed[i] = 0;
<//><//><//>/>)<//>dnl
<//><//>/>)<//>dnl

}

    // this is where we put the serialized data we have extracted from the hash table
    SerializedSegmentArray storage;

    // this is the new hash table segment we are building
    HashTableSegment newSegment;
    newSegment.Allocate ();

    // this is the last slot in the segment that we have zeroed out
    HT_INDEX_TYPE upperBound = 0;

    // Before we start main for loop, mark all start boundaries of fragment
    for (int IDX = 0; IDX < numDyingWaypoints; IDX++) {
        bitmapColumnIterLHS[IDX].MarkFragment (true);
        hashColumnIterLHS[IDX].MarkFragment ();
        bitmapColumnIterRHS[IDX].MarkFragment (true);
        hashColumnIterRHS[IDX].MarkFragment ();
<//><//>m4_foreach(</_A_/>,m4_quote(reval(</m4_args/>M4_LHS_Attr)),</dnl
<//><//><//>M4_IFVALID_ATT(_A_, </dnl
        _A_<//>_ColumnIter_LHS[IDX].MarkFragment ();
<//><//><//>/>)<//>dnl
<//><//>/>)<//>dnl
<//><//>m4_foreach(</_A_/>,m4_quote(reval(</m4_args/>M4_RHS_Attr)),</dnl
<//><//><//>M4_IFVALID_ATT(_A_, </dnl
        _A_<//>_ColumnIter_RHS[IDX].MarkFragment ();
<//><//><//>/>)<//>dnl
<//><//>/>)<//>dnl
    }

    // now, loop through the hash table!
    for (HT_INDEX_TYPE i = 0; i < NUM_SLOTS_IN_SEGMENT; i++) {

        HT_INDEX_TYPE curSlot = i;
        while (1) {

            // we zero out the segment incrementally, as we write it... this is the number
            // of slots that we zero out in one fell swoop
#define ZEROING_OUT_STEP_SIZE 2048
            if (curSlot + ZEROING_OUT_STEP_SIZE > upperBound)  {
                HT_INDEX_TYPE newUpperBound = curSlot + ZEROING_OUT_STEP_SIZE;
                if (newUpperBound >= NUM_SLOTS_IN_SEGMENT)
                    newUpperBound = ABSOLUTE_HARD_CAP;
                newSegment.ZeroOut (upperBound, newUpperBound);
                upperBound = newUpperBound;
            }

            // begin by trying to extract the bitmap
            int whichWayPoint = -1, LHS, columnID, dummy, done;
            Bitstring bitstringIFound;
            int lastLen = mySegment.Extract (&bitstringIFound, curSlot, i, whichWayPoint, BITMAP, LHS, done);


            // see if we didn't find any data in this slot
            if (lastLen == 0)
                break;

            // we did find data, so get this guy's bitmap
            Bitstring *bitstringPtr = &bitstringIFound;

            // determine what is going on with this waypoint
            int state = wayPointInfo[whichWayPoint].isDying;
            int index = wayPointInfo[whichWayPoint].index;
            Bitstring deadQueries = wayPointInfo[whichWayPoint].killThese;
            bitstringPtr->Difference (deadQueries);

            if (state == DYING_AND_SEND && LHS) {

                // first put the bitmap in
                bitmapColumnIterLHS[index].Insert (*bitstringPtr);
                bitmapColumnIterLHS[index].Advance ();
                bitmapColumnLHSIsUsed[index]++;
                counter++;

                // now put the hash in
                hashColumnIterLHS[index].Insert (i);
                hashColumnIterLHS[index].Advance ();

                // if this guy is dying and we got RHS data, put the bitmap (and hash) in a RHS chunk
            } else if (state == DYING_AND_SEND && !LHS) {

                // first put the bitmap in
                bitmapColumnIterRHS[index].Insert (*bitstringPtr);
                bitmapColumnIterRHS[index].Advance ();
                bitmapColumnRHSIsUsed[index]++;

                // now put the hash in
                hashColumnIterRHS[index].Insert (i);
                hashColumnIterRHS[index].Advance ();

                // if this entry has interesting data, then put it in
            } else if (!bitstringPtr->IsEmpty () && state == ALIVE) {

                storage.StartNew (i, whichWayPoint, !LHS, bitstringPtr, lastLen);
            }

            // if we finished the tuple, stop serializing
            if (done)
                goto end;

            // NOW DO LHS COLUMNS
<//><//>m4_foreach(</_A_/>,m4_quote(reval(</m4_args/>M4_LHS_Attr)),</dnl
<//><//><//>M4_IFVALID_ATT(_A_, </dnl
            columnID = M4_ATT_SLOT(_A_);
            lastLen = mySegment.Extract (serializeHere, curSlot, i, whichWayPoint, columnID, dummy, done);

            // see if we got something... in the first two cases, we are extracting data for a disk-based join
            if (lastLen > 0 && state == DYING_AND_SEND && LHS) {

                // first, make sure that we are far enough along
                for (; _A_<//>_Column_LHSIsUsed[index] < bitmapColumnLHSIsUsed[index] - 1; _A_<//>_Column_LHSIsUsed[index]++) {
                    M4_ATT_TYPE(_A_) col; // TBD, some default value
                _A_<//>_ColumnIter_LHS[index].Insert (col);
                _A_<//>_ColumnIter_LHS[index].Advance ();
            }

            // now put the data in
            M4_ATT_TYPE(_A_) *_A_<//>_ColumnPtr = ((M4_ATT_TYPE(_A_) *) (serializeHere));
            _A_<//>_ColumnIter_LHS[index].Insert (*_A_<//>_ColumnPtr);
            _A_<//>_ColumnIter_LHS[index].Advance ();
            _A_<//>_Column_LHSIsUsed[index]++;

            } else if (lastLen > 0 && state == DYING_AND_SEND && !LHS) {

                FATAL ("<//>_A_ is only used by LHS... how did I find it in a RHS tuple?\n");

                // in the last case, the data will go back into the hash table
            } else if (lastLen > 0 && !bitstringPtr->IsEmpty () && state == ALIVE) {

                storage.Append (columnID, serializeHere, lastLen);
            }

            // if we finished the tuple, stop serializing
            if (done)
                goto end;
<//><//><//>/>)<//>dnl
<//><//>/>)<//>dnl

            // NOW DO RHS COLUMNS
<//><//>m4_foreach(</_A_/>,m4_quote(reval(</m4_args/>M4_RHS_Attr)),</dnl
<//><//><//>M4_IFVALID_ATT(_A_, </dnl
            columnID = M4_ATT_SLOT(_A_);
            lastLen = mySegment.Extract (serializeHere, curSlot, i, whichWayPoint, columnID, dummy, done);

            // see if we got something... in the first two cases, we are extracting data for a disk-based join
            if (lastLen > 0 && state == DYING_AND_SEND && LHS) {

                FATAL ("<//>_A_ is only used by RHS... how did I find it in a LHS tuple?\n");

            } else if (lastLen > 0 && state == DYING_AND_SEND && !LHS) {

                // first, make sure that we are far enough along
                for (; _A_<//>_Column_RHSIsUsed[index] < bitmapColumnRHSIsUsed[index] - 1; _A_<//>_Column_RHSIsUsed[index]++) {
                    M4_ATT_TYPE(_A_) col; // TBD, some default value
                _A_<//>_ColumnIter_RHS[index].Insert (col);
                _A_<//>_ColumnIter_RHS[index].Advance ();
            }

            // now put the data in
            M4_ATT_TYPE(_A_) *_A_<//>_ColumnPtr = ((M4_ATT_TYPE(_A_) *) (serializeHere));
            _A_<//>_ColumnIter_RHS[index].Insert (*_A_<//>_ColumnPtr);
            _A_<//>_ColumnIter_RHS[index].Advance ();
            _A_<//>_Column_RHSIsUsed[index]++;

            // in the last case, the data will go back into the hash table
            } else if (lastLen > 0 && !bitstringPtr->IsEmpty () && state == ALIVE) {

                storage.Append (columnID, serializeHere, lastLen);
            }

            // if we finished the tuple, stop serializing
            if (done)
                goto end;
<//><//><//>/>)<//>dnl
<//><//>/>)<//>dnl

end:        // AT THIS POINT, DONE WITH ALL OF THE COLUMNS!!

            // in this case, we have some data to put back into the new hash table
            if (!bitstringIFound.IsEmpty () && state == ALIVE) {
                newSegment.Insert (storage);
                storage.EmptyOut ();

            }
        }
        // Mark only if i is not at the end and condition matches for all LSBs as 11111111....
        if ((i != NUM_SLOTS_IN_SEGMENT-1) && (i & ((1ULL << (NUM_SLOTS_IN_SEGMENT_BITS-NUM_FRAGMENT_BITS))- 1ULL)) == ((1ULL<<(NUM_SLOTS_IN_SEGMENT_BITS-NUM_FRAGMENT_BITS))-1ULL)) {
            //printf("\n Markfragment Index value = %d  %lx", i, &i); fflush(stdout);
            //if (i == NUM_SLOTS_IN_SEGMENT-1) assert(0);
            for (int IDX = 0; IDX < numDyingWaypoints; IDX++) {
                bitmapColumnIterLHS[IDX].MarkFragment (false);
                hashColumnIterLHS[IDX].MarkFragment ();
                bitmapColumnIterRHS[IDX].MarkFragment (false);
                hashColumnIterRHS[IDX].MarkFragment ();
<//><//>m4_foreach(</_A_/>,m4_quote(reval(</m4_args/>M4_LHS_Attr)),</dnl
<//><//><//>M4_IFVALID_ATT(_A_, </dnl
                _A_<//>_ColumnIter_LHS[IDX].MarkFragment ();
<//><//><//>/>)<//>dnl
<//><//>/>)<//>dnl
<//><//>m4_foreach(</_A_/>,m4_quote(reval(</m4_args/>M4_RHS_Attr)),</dnl
<//><//><//>M4_IFVALID_ATT(_A_, </dnl
                _A_<//>_ColumnIter_RHS[IDX].MarkFragment ();
<//><//><//>/>)<//>dnl
<//><//>/>)<//>dnl
            }
        }
    }

    // Mark all the last guys, just the tuples
    for (int IDX = 0; IDX < numDyingWaypoints; IDX++) {
        bitmapColumnIterLHS[IDX].MarkFragmentTuples ();
        bitmapColumnIterRHS[IDX].MarkFragmentTuples ();
    }

    PROFILING(0.0, "Cleaner", "FindTuples", "%d", counter);
    newSegment.ZeroOut (upperBound, ABSOLUTE_HARD_CAP);


    // now we are at the final cleanup, where we build our output chunks... there could potentially be one
    // LHS chunk and one RHS chunk for each and every join waypoint

    // this will hold all of the chunks we produce
    ExtractionList extractionResult;

    // first we see if there are any LHS chunks to build
    for (int i = 0; i < numDyingWaypoints; i++) {

        // declare one column for each LHS attribute
        Column hash;
        //Column bitmap;

        // kill the iterators for the hash and bitmap
        hashColumnIterLHS[i].Done (hash);
        //bitmapColumnIterLHS[i].Done (bitmap);
        bitmapColumnIterLHS[i].Done ();

        // this is the chunk we're building
        Chunk myChunk;

        // and see if we need to build a chunk
        if (bitmapColumnLHSIsUsed[i]) {

            // put the bitmap and the hash column in
            //myChunk.SwapBitmap (bitmap);
            myChunk.SwapBitmap (bitmapColumnIterLHS[i]);
            myChunk.SwapHash (hash);

            // now add the atts, one-at-a-time, if valid
<//><//>m4_foreach(</_A_/>,m4_quote(reval(</m4_args/>M4_LHS_Attr)),</dnl
<//><//><//>M4_IFVALID_ATT(_A_, </dnl
            if (_A_<//>_Column_LHSIsUsed[i]) {

                M4_ATT_TYPE(_A_) colDummyVal;

                // first, make sure that we are far enough along
                for (; _A_<//>_Column_LHSIsUsed[i] < bitmapColumnLHSIsUsed[i]; _A_<//>_Column_LHSIsUsed[i]++) {
                    _A_<//>_ColumnIter_LHS[i].Insert (colDummyVal);
                    _A_<//>_ColumnIter_LHS[i].Advance ();
                }

                Column col;
                _A_<//>_ColumnIter_LHS[i].Done (col);
                myChunk.SwapColumn (col, M4_ATT_SLOT(_A_));
            }
<//><//><//>/>)<//>dnl
<//><//>/>)<//>dnl

            // at this point, we have totally constructed the LHS chunk, so package it up
            // first, find out which of the join waypoints was put in this slot
            unsigned int whichJoin = 0;
            for (; wayPointInfo[whichJoin].index != i; whichJoin++);

            // now, find the disk waypoint that gets this guy's info
            WayPointID whichWayPoint;
            for (myWork.get_equivalences ().MoveToStart (); 1; myWork.get_equivalences ().Advance ()) {
                if (myWork.get_equivalences ().Current ().joinWayPointID == whichJoin) {
                    whichWayPoint = myWork.get_equivalences ().Current ().diskBasedTwinID;
                    break;
                }
            }

            // and put the info in
            int isLHS = 1;
            int whichSegment = whichSegment;
            ExtractedChunk myResult (isLHS, whichWayPoint, myChunk);
            extractionResult.Insert (myResult);
        }
    }

    // now we see if there are any RHS chunks to build
    for (int i = 0; i < numDyingWaypoints; i++) {

        // declare one column for each RHS attribute
        Column hash;
        //Column bitmap;

        // kill the iterators
        hashColumnIterRHS[i].Done (hash);
        bitmapColumnIterRHS[i].Done ();

        // this is the chunk we're building
        Chunk myChunk;

        // and see if we need to build a chunk
        if (bitmapColumnRHSIsUsed[i]) {

            // put the bitmap and the hash column in
            myChunk.SwapBitmap (bitmapColumnIterRHS[i]);
            myChunk.SwapHash (hash);

            // now add the atts, one-at-a-time, if valid
<//><//>m4_foreach(</_A_/>,m4_quote(reval(</m4_args/>M4_RHS_Attr)),</dnl
<//><//><//>M4_IFVALID_ATT(_A_, </dnl
            if (_A_<//>_Column_RHSIsUsed[i]) {

                M4_ATT_TYPE(_A_) colDummyVal;

                // first, make sure that we are far enough along
                for (; _A_<//>_Column_RHSIsUsed[i] < bitmapColumnRHSIsUsed[i]; _A_<//>_Column_RHSIsUsed[i]++) {
                    _A_<//>_ColumnIter_RHS[i].Insert (colDummyVal);
                    _A_<//>_ColumnIter_RHS[i].Advance ();
                }

                Column col;
                _A_<//>_ColumnIter_RHS[i].Done (col);
                myChunk.SwapColumn (col, M4_ATT_SLOT(_A_));
            }
<//><//><//>/>)<//>dnl
<//><//>/>)<//>dnl

            // first, find out which of the join waypoints was put in this slot
            unsigned int whichJoin = 0;
            for (; wayPointInfo[whichJoin].index != i; whichJoin++);

            // now, find the disk waypoint that gets this guy's info
            WayPointID whichWayPoint;
            for (myWork.get_equivalences ().MoveToStart (); 1; myWork.get_equivalences ().Advance ()) {
                if (myWork.get_equivalences ().Current ().joinWayPointID == whichJoin) {
                    whichWayPoint = myWork.get_equivalences ().Current ().diskBasedTwinID;
                    break;
                }
            }

            // at this point, we have totally constructed the RHS chunk, so package it up
            int isLHS = 0;
            int whichSegment = whichSegment;
            ExtractedChunk myResult (isLHS, whichWayPoint, myChunk);
            extractionResult.Insert (myResult);
        }
    }

    // and get outta here!
    ExtractionContainer finalResult (whichSegment, myWork.get_diskTokenQueue (), extractionResult);
    ExtractionResult reallyFinalResult (whichSegment, newSegment, finalResult);
    reallyFinalResult.swap (result);

    free (serializeHere);
    delete [] wayPointInfo;

    return 1;
}
