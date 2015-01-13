<?

/* Copyright Tera Insights, 2013, All Rights Reserved

*/

function JoinMerge($wpName, $jDesc) {

?>


#define USE_PQ

#ifdef USE_PQ
//#include <algorithm>
#include <vector>
#else
#include <vector>
#include<set>
#endif

#include "WorkDescription.h"
#include "ExecEngineData.h"
#include "Column.h"
#include "ColumnIterator.cc"
#include "MMappedStorage.h"
#include "BString.h"
#include "BStringIterator.h"
#include "HashTableMacros.h"

#include <string.h>

using namespace std;

#define MAX_HASH 0xffffffffffff
#define MAX_NUM_CHUNKS 128 // maximum number of chunks we can have for linear scan solution

#ifndef USE_PQ
struct HashWrapper {
    HashWrapper() : hash(MAX_HASH), chunkNo(0) {}
    __uint64_t hash:48;
    __uint64_t chunkNo:16;
};

#endif

<?  // Macro to find minimum value in a list
    // this is the loop that finds the best entry using linear scan
    // # minIndex$1 is set at the best position
    // in put assumed in hashes$1
    // Arguments:
    // $side is "LHS" or "RHS"
    $FIND_MIN = function($side) { ?>
    best<?=$side?>= MAX_HASH;
    for (int i=0; i < num<?=$side?>Chunks; i++) {
        HT_INDEX_TYPE hash = hashes<?=$side?>[i]; // current value
        minIndex<?=$side?> = (best<?=$side?> > hash) ? i : minIndex<?=$side?>;
        best<?=$side?> = (best<?=$side?> > hash) ? hash: best<?=$side?>;
    }
    // remove this after debugging
    FATALIF(best<?=$side?> != MAX_HASH && WHICH_SEGMENT(best<?=$side?>) !=0,
    "Hash out of range %llu", (unsigned long long) best<?=$side?>);
<? }; ?>

/*
void check_correctness(int start, int end, BStringIterator& biter, ColumnIterator<__uint64_t>& hiter) {
    assert(start == end);
    int s = 0;
    int e = 262143;
    if (start > 0) {
        s = start * 262143 + (start-1);
        e = s + 262143 + 1;
    }
    int tuple = 0;
    while (!biter.AtEndOfColumn()) {
        if (!(s < hiter.GetCurrent() && hiter.GetCurrent() <= e))
            printf("\n -------   (%d,%d)(%d,%d)  currVal = %d", start,end,s,e,hiter.GetCurrent());
            assert(s <= hiter.GetCurrent() && hiter.GetCurrent() <= e);
            hiter.Advance();
            biter.Advance();
            tuple++;
        }
        printf("\n num tuples = %d", tuple);
    }
*/

#ifndef USE_PQ
// make heap comparator
struct compare_key {
    bool operator()( const HashWrapper lhs, const HashWrapper rhs )
    {
        return (rhs.hash < lhs.hash);
    }
};

struct compare_key_rev {
    bool operator()( const HashWrapper lhs, const HashWrapper rhs )
    {
        return (lhs.hash < rhs.hash);
    }
};
#endif


<? // More macros

    // Macro to Advance() columns
    // $side is LHS or RHS
    // $chunkNum is the chunk number
    // $sde is Lhs or Rhs
    $ADVANCE_CALL = function($side, $chunkNum, $sde) use ($jDesc){
        $list = "attribute_queries_".$side;
        foreach($jDesc->$list as $att => $queries){ ?>
    col<?=$side?>IterVec_<?=$att?>[<?=$chunkNum?>].Advance();
    <?  } ?>
    myInBStringIter<?=$sde?>Vec[<?=$chunkNum?>].Advance();
    col<?=$side?>IterVecHash[<?=$chunkNum?>].Advance();
<?  };

    $ADVANCE_SEARCH_CALL = function($col, $index, $side, $sde) use ($jDesc, $FIND_MIN){
        $list = "attribute_queries_".$side;
?>
    //while (!myInBStringIter$4Vec[$2].AtEndOfColumn())
    if (!myInBStringIter<?=$sde?>Vec[<?=$index?>].AtEndOfColumn()) {
    //while (!<?=$col?>[<?=$index?>].AtUnwrittenByte())
        // Now find the first tuple for which query is active
        //Bitstring curBits(myInBStringIterLhsVec[<?=$index?>].GetCurrent ());
        //curBits.Intersect (queriesToRun);
        // If tuple has some active query, fill the heap
        //if (!curBits.IsEmpty())
        totalguys<?=$side?>++;
#ifdef USE_PQ
        // set new value in place of old value >
        // ALIN: Please fix the line below, I don't know what the $1 is supposed to be '
        hashes<?=$side?>[minIndex<?=$side?>]  = <?=$col?>[minIndex<?=$side?>].GetCurrent();
#else
        HashWrapper w;
        w.hash = <?=$col?>[<?=$index?>].GetCurrent(); // assume first column of each chunk is hash
        w.chunkNo = <?=$index?>;

        minHeap<?=$side?>.insert (w);
#endif
/*
        break; // while loop
    } else {
        // If this tuple don't have active query, advance all the columns of this chunk '
        // including Bitstring column
<? foreach($jDesc->$list as $att){ ?>
        col<?=$side?>IterVec_<?=$att?>[<?=$index?>].Advance();
<? } /* foreach */ ?>
        myInBStringIter<?=$sde?>Vec[<?=$index?>].Advance();
        col<?=$side?>IterVecHash[<?=$index?>].Advance();
    }
*/
    }
#ifdef USE_PQ
    <?$FIND_MIN($side);?>
#endif
<? }; /* ADVANCE_SEARCH_CALL */ ?>

/*
    This takes two sorted list of chunks, lhs list and rhs list, and do the sort merge join.
    It maintaines a min heap of hash values on top of each list and virtually now we have
    just 2 lists to do sort merge. When some value matches from LHS and RHS heap, we need
    to checkpoint all the iterators of all the columns of all the RHS chunks including storing
    the heap, so that we can restore them all if another consecutive LHS value matches.
*/

//+{"kind":"WPF", "name":"Merge", "action":"start"}
extern "C"
int JoinMergeWorkFunc_<?=$wpName?>_writer (WorkDescription &workDescription, ExecEngineData &result) {

int totalguysLHS = 0;
int totalguysRHS = 0;
int total = 0;
int totalhash = 0;

    double start_time = global_clock.GetTime();

    // get the input LHS and RHS chunk container from work descripton
    JoinMergeWorkDescription myWork;
    myWork.swap (workDescription);
    ContainerOfChunks &inputLHSList = myWork.get_chunksLHS ();
    ContainerOfChunks &inputRHSList = myWork.get_chunksRHS ();

    int start = myWork.get_start();
    int end = myWork.get_end();

    // get the number of chunks in each list to create vectors of this length
    int numLHSChunks = inputLHSList.Length();
    int numRHSChunks = inputRHSList.Length();

    // get the waypoint identifier
    unsigned int wayPointID = myWork.get_wayPointID ().GetID();

    vector<ColumnIterator<__uint64_t> > colLHSIterVecHash;
    vector<ColumnIterator<__uint64_t> > colRHSIterVecHash;
    colLHSIterVecHash.resize(numLHSChunks);
    colRHSIterVecHash.resize(numRHSChunks);

    // get the input bitmap out of the LHS input chunks
    vector<BStringIterator> myInBStringIterLhsVec;
    myInBStringIterLhsVec.resize(numLHSChunks); // set the vector size
    assert(inputLHSList.Length());
    inputLHSList.MoveToStart();
    int i = 0;
    while (inputLHSList.RightLength()) {
        BStringIterator myInBStringIter;
        inputLHSList.Current().SwapBitmap (myInBStringIter);
        myInBStringIterLhsVec[i].swap (myInBStringIter);
        myInBStringIterLhsVec[i].SetFragmentRange(start, end);

        // get the hash now
        Column col_hash;
        inputLHSList.Current().SwapHash(col_hash);
        assert(col_hash.IsValid());
        ColumnIterator<__uint64_t> iter (col_hash, start, end);
        colLHSIterVecHash[i].swap(iter);
        //check_correctness(start, end, myInBStringIterLhsVec[i], colLHSIterVecHash[i]);
        inputLHSList.Advance ();
        i++;
    }

    // get the input bitmap out of the RHS input chunks
    vector<BStringIterator> myInBStringIterRhsVec;
    myInBStringIterRhsVec.resize(numRHSChunks); // set the vector size
    assert(inputRHSList.Length());
    inputRHSList.MoveToStart();
    i = 0;
    while (inputRHSList.RightLength()) {
        BStringIterator myInBStringIter;
        inputRHSList.Current().SwapBitmap(myInBStringIter);
        myInBStringIterRhsVec[i].swap (myInBStringIter);
        myInBStringIterRhsVec[i].SetFragmentRange(start, end);
        //fprintf(stderr, "\nRHS %d", myInBStringIterRhsVec[i].GetNumTuples());
        // get the hash now
        Column col_hash;
        inputRHSList.Current().SwapHash(col_hash);
        assert(col_hash.IsValid());
        ColumnIterator<__uint64_t> iter (col_hash, start, end);
        colRHSIterVecHash[i].swap(iter);
        //check_correctness(start, end, myInBStringIterRhsVec[i], colRHSIterVecHash[i]);
        inputRHSList.Advance ();
        i++;
    }

    // get all of the queries that are active here
    QueryIDSet queriesToRun = QueryExitsToQueries(myWork.get_whichQueryExits ());

    // this is the output chunk
    Chunk output;

    // create output iterators
<?  foreach($jDesc->attribute_queries_LHS_copy as $att => $queries){ ?>
    MMappedStorage store_<?=$att?>;
    Column  col_<?=$att?>(store_<?=$att?>);
    <?=attIteratorType($att)?>  colLHSOutIter_<?=$att?>(col_<?=$att?>);
<?  } /*foreach*/ ?>

<?  foreach($jDesc->attribute_queries_RHS_copy as $att => $queries){ ?>
    MMappedStorage store_<?=$att?>;
    Column  col_<?=$att?>(store_<?=$att?>);
    <?=attIteratorType($att)?>  colRHSOutIter_<?=$att?>(col_<?=$att?>);
<?  } /*foreach*/ ?>

    // Create output BitString
    MMappedStorage myStore;
    Column bitmapOut (myStore);
    BStringIterator myOutBStringIter (bitmapOut, queriesToRun);

    // Build input iterators vectors first
    // Define only those attributes iterator which are required by some query
    // vector contains same type of column in all chunks, each vector index represents
    // a chunk
<?  foreach($jDesc->attribute_queries_LHS as $att => $queries){ ?>
    vector< <?=attIteratorType($att)?> > colLHSIterVec_<?=$att?>;
    colLHSIterVec_<?=$att?>.resize(numLHSChunks);
<?  } /* foreach */ ?>

<?  foreach($jDesc->attribute_queries_RHS as $att => $queries){ ?>
    vector< <?=attIteratorType($att)?> > colRHSIterVec_<?=$att?>;
    colRHSIterVec_<?=$att?>.resize(numRHSChunks);
<?  } /* foreach */ ?>

    // Extract columns now.
    // This extracts columns if there is any query for it in this WP
    i = 0;
<?  foreach($jDesc->attribute_queries_LHS as $att => $queries){ ?>
    {
        i = 0;
        inputLHSList.MoveToStart();
        while (inputLHSList.RightLength()) {
            // WP node queries intersect Translator provided queries
            QueryIDSet <?=attQrys($att)?>(<?=$queries?>, true);
<? cgExtractColumnFragment($att, "inputLHSList.Current()", "start", "end", $wpName); ?>
            if (<?=attQrys($att)?>.Overlaps(queriesToRun)){
                colLHSIterVec_<?=$att?>[i].swap(<?=attData($att)?>);
            }
            inputLHSList.Advance ();
            i++;
        }
    }
<?  } /* foreach */ ?>

<?  foreach($jDesc->attribute_queries_RHS as $att => $queries){ ?>
    {
        i = 0;
        inputRHSList.MoveToStart();
        while (inputRHSList.RightLength()) {
            // WP node queries intersect Translator provided queries
            QueryIDSet <?=attQrys($att)?>(<?=$queries?>, true);
<? cgExtractColumnFragment($att, "inputRHSList.Current()", "start", "end", $wpName); ?>
            if (<?=attQrys($att)?>.Overlaps(queriesToRun)){
                colRHSIterVec_<?=$att?>[i].swap(<?=attData($att)?>);
            }
            inputRHSList.Advance ();
            i++;
        }
    }
<?  } /* foreach */ ?>


    // Here we start the sort merge join
    // Create priority queue for hash column values
#ifdef USE_PQ
    HT_INDEX_TYPE hashesLHS[MAX_NUM_CHUNKS];
    HT_INDEX_TYPE hashesRHS[MAX_NUM_CHUNKS];
    FATALIF(numLHSChunks >= MAX_NUM_CHUNKS, "Too many chunks for LHS in Merge");
    FATALIF(numRHSChunks >= MAX_NUM_CHUNKS, "Too many chunks for RHS in Merge");
    HT_INDEX_TYPE bestLHS = MAX_HASH;
    HT_INDEX_TYPE bestRHS = MAX_HASH;
    int minIndexLHS = -1; // this indicates the chunk that is the best at this point
    // this invariant is maintained throught the code
    int minIndexRHS = -1;
#else
    multiset<HashWrapper, compare_key_rev> minHeapLHS;
    multiset<HashWrapper, compare_key_rev> minHeapRHS;
#endif

    // Fill the first value of each LHS chunk in LHS heap
    for (i = 0; i < numLHSChunks; i++) {
#ifdef USE_PQ
        minIndexLHS = i;
#endif
<?      $ADVANCE_SEARCH_CALL("colLHSIterVecHash","i","LHS","Lhs"); ?>
    }

    // Fill the first value of each RHS chunk in RHS heap
    for (i = 0; i < numRHSChunks; i++) {
#ifdef USE_PQ
        minIndexRHS = i;
#endif
<?      $ADVANCE_SEARCH_CALL("colRHSIterVecHash","i","RHS","Rhs"); ?>
    }

    // Now pick one of each heap and keep comparing until one of the heap is exhausted
#ifdef USE_PQ
/*    while (hashesLHS[minIndexLHS] != MAX_HASH && hashesRHS[minIndexRHS] != MAX_HASH) {
        HT_INDEX_TYPE wl = hashesLHS[minIndexLHS];
        HT_INDEX_TYPE wr = hashesRHS[minIndexRHS]; -- INEFFICIENT */

    while (bestLHS != MAX_HASH && bestRHS != MAX_HASH) {
        HT_INDEX_TYPE wl = bestLHS;
        HT_INDEX_TYPE wr = bestRHS;

#else
    while (!minHeapLHS.empty() && !minHeapRHS.empty()) {
        HashWrapper wlT = *(minHeapLHS.begin());
        HashWrapper wrT = *(minHeapRHS.begin());
        HT_INDEX_TYPE wl = wlT.hash;
        HT_INDEX_TYPE wr = wrT.hash;
        int minIndexLHS = wlT.chunkNo;
        int minIndexRHS = wrT.chunkNo;
#endif

        if (wl < wr) {
            //    printf("\n Hash val not found is = %ld, segment = %ld, slot = %ld", wl, WHICH_SEGMENT (wl), WHICH_SLOT (wl)); fflush(stdout); assert(0);

            // erase the minimum element
#ifdef USE_PQ
            hashesLHS[minIndexLHS] = MAX_HASH;
#else
            assert (!minHeapLHS.empty());
            minHeapLHS.erase(minHeapLHS.begin());
#endif

            // here advance all columns of LHS of chunk number wl.chunkNo
<?          $ADVANCE_CALL("LHS","minIndexLHS","Lhs");
            $ADVANCE_SEARCH_CALL("colLHSIterVecHash","minIndexLHS","LHS","Lhs"); ?>

        } else if (wl > wr) {

            // erase the minimum element
#ifdef USE_PQ
            hashesRHS[minIndexRHS] = MAX_HASH;
#else
            assert (!minHeapRHS.empty());
            minHeapRHS.erase(minHeapRHS.begin());
#endif

            // here advance all columns of RHS of chunk number wr.chunkNo
<?          $ADVANCE_CALL("RHS","minIndexRHS","Rhs");
            $ADVANCE_SEARCH_CALL("colRHSIterVecHash","minIndexRHS","RHS","Rhs");
?>

        } else { // (wl == wr)

            HT_INDEX_TYPE matchingHash = wl;

            // Save checkpoint for all rhs columns before incrementing both chunks
            for (int chk = 0; chk < numRHSChunks; chk++) {
<?  foreach($jDesc->attribute_queries_RHS as $att => $queries){ ?>
                    colRHSIterVec_<?=$att?>[chk].CheckpointSave();
<?  } /* foreach */ ?>
                myInBStringIterRhsVec[chk].CheckpointSave();
                colRHSIterVecHash[chk].CheckpointSave();
            }

            // Also save the state of the heap
#ifdef USE_PQ
            HT_INDEX_TYPE hashesRHS_copy[MAX_NUM_CHUNKS];
            memcpy(hashesRHS_copy, hashesRHS, numRHSChunks*sizeof(HT_INDEX_TYPE));
            int minIndexRHS_copy = minIndexRHS;
#else
            multiset<HashWrapper, compare_key_rev> minHeapRHSCheckpoint(minHeapRHS);
#endif

#ifdef USE_PQ
            while (bestLHS != MAX_HASH)
#else
            while (!minHeapLHS.empty()) // break from this if 2 consecutive LHS mismatches
#endif
	    {
#ifdef USE_PQ
//                HT_INDEX_TYPE wl1 = hashesLHS[minIndexLHS];
                HT_INDEX_TYPE wl1 = bestLHS;
#else
                HashWrapper wl1T = *(minHeapLHS.begin());
                HT_INDEX_TYPE wl1 = wl1T.hash;
                int minIndexLHS = wl1T.chunkNo;
#endif

                if (wl1 != matchingHash) { // next LHS dont match to previous LHS hash, first time always match
                    break;

                } else { // restore everything

                    // restore the RHS Column iterators to original value
                    for (int chk = 0; chk < numRHSChunks; chk++) {
<?  foreach($jDesc->attribute_queries_RHS as $att => $queries){ ?>
                            colRHSIterVec_<?=$att?>[chk].CheckpointRestore();
<?  } /* foreach */ ?>
                            myInBStringIterRhsVec[chk].CheckpointRestore();
                            colRHSIterVecHash[chk].CheckpointRestore();
                    }
                    // restore the original heap state
#ifdef USE_PQ
                    memcpy(hashesRHS, hashesRHS_copy,  numRHSChunks*sizeof(HT_INDEX_TYPE));
                    minIndexRHS = minIndexRHS_copy;
                    bestRHS = hashesRHS[minIndexRHS_copy];
#else
                    minHeapRHS.clear();
                    minHeapRHS = minHeapRHSCheckpoint;
#endif
                }

#ifdef USE_PQ
//                while (hashesRHS[minIndexRHS] != MAX_HASH) {
                while (bestRHS != MAX_HASH) {
#else
                while (!minHeapRHS.empty()) {
#endif

#ifdef USE_PQ
//                    HT_INDEX_TYPE wr1 = hashesRHS[minIndexRHS];
                    HT_INDEX_TYPE wr1 = bestRHS;
#else
                    HashWrapper wr1T = *(minHeapRHS.begin());
                    HT_INDEX_TYPE wr1 = wr1T.hash;
                    int minIndexRHS = wr1T.chunkNo;
#endif

                    if (wl1 == wr1) { // first one will obviously match as it matched before
                        // Merge all columns here for wl1.chunkNo and wr1.chunkNo after matching attributes

                        // Make sure both of their bitstrings intersect
                        Bitstring rez = queriesToRun;
                        rez.Intersect (myInBStringIterRhsVec[minIndexRHS].GetCurrent ());
                        rez.Intersect (myInBStringIterLhsVec[minIndexLHS].GetCurrent ());

                        // This contains union of all translater queries
                        Bitstring uni = 0;
                        Bitstring qBits;
                        totalhash++;


                        // Do the actual comparision
                        bool anyOneMatch = false;
<? foreach($jDesc->queries_attribute_comparison as $qClass) { ?>
                            // See if any query in query class is eligible for this comparision
                            qBits = QueryIDSet(<?=$qClass->qClass?>, true);
                            qBits.Intersect(rez);
                            if (
                            /*    !qBits.IsEmpty () && */
    <? foreach($qClass->att_pairs as $pair) { ?>
                                colLHSIterVec_<?=$pair->lhs?>[minIndexLHS].GetCurrent() == colRHSIterVec_<?=$pair->rhs?>[minIndexRHS].GetCurrent() &&
    <? } /*foreach pair*/ ?>
                                1 ) {
                                anyOneMatch = true;
                                uni.Union (qBits);
                            }
<? } /* foreach qClass */ ?>

                        if (anyOneMatch) {
                            // fill the output iterators
<?  foreach($jDesc->attribute_queries_LHS_copy as $att => $queries){ ?>
                                //if (rez.Overlaps(<?=$queries?>)) { Should we check translator queries?
                                    colLHSOutIter_<?=$att?>.Insert(colLHSIterVec_<?=$att?>[minIndexLHS].GetCurrent());
                                    colLHSOutIter_<?=$att?>.Advance();
                                //}
<?  } /* foreach */ ?>


<?  foreach($jDesc->attribute_queries_RHS_copy as $att => $queries){ ?>
                                //if (rez.Overlaps(<?=$queries?>)) { Should we check translator queries?
                                    colRHSOutIter_<?=$att?>.Insert(colRHSIterVec_<?=$att?>[minIndexRHS].GetCurrent());
                                    colRHSOutIter_<?=$att?>.Advance();
                                //}
<?  } /* foreach */ ?>

                            myOutBStringIter.Insert (uni);
                            total++;
                            myOutBStringIter.Advance ();
                        } else {
                            //Bitstring b(0, true);
                            //myOutBStringIter.Insert (b);
                            //myOutBStringIter.Advance ();
                        }

                        // erase the minimum element
#ifdef USE_PQ
                        hashesRHS[minIndexRHS] = MAX_HASH;
#else
                        minHeapRHS.erase(minHeapRHS.begin());
#endif

                        // here advance all columns of RHS of chunk number wr1.chunkNo
<?                      $ADVANCE_CALL("RHS","minIndexRHS","Rhs");
                        $ADVANCE_SEARCH_CALL("colRHSIterVecHash","minIndexRHS","RHS","Rhs"); ?>
                    } else {
                        break;
                    }
                }

                // erase the minimum element
#ifdef USE_PQ
                hashesLHS[minIndexLHS] = MAX_HASH;
#else
                minHeapLHS.erase(minHeapLHS.begin());
#endif

                // here advance all columns of LHS of chunk number wl1.chunkNo
<?              $ADVANCE_CALL("LHS","minIndexLHS","Lhs");
                $ADVANCE_SEARCH_CALL("colLHSIterVecHash","minIndexLHS","LHS","Lhs"); ?>
            }
        }
    }

    // fill the output iterators
<?  foreach($jDesc->attribute_queries_LHS_copy as $att => $queries){ ?>
    Column collhs_<?=$att?>;
    colLHSOutIter_<?=$att?>.Done(collhs_<?=$att?>);
    output.SwapColumn (collhs_<?=$att?>, <?=attSlot($att)?>);
<?  } /* foreach */ ?>
<?  foreach($jDesc->attribute_queries_RHS_copy as $att => $queries){ ?>
    Column colrhs_<?=$att?>;
    colRHSOutIter_<?=$att?>.Done(colrhs_<?=$att?>);
    output.SwapColumn (colrhs_<?=$att?>, <?=attSlot($att)?>);
<?  } /* foreach */ ?>

    //myOutBStringIter.Done (bitmapOut);
    myOutBStringIter.Done ();
//    printf("\nTuples %d %d %d %d %d", myOutBStringIter.GetNumTuples(), total, totalguysLHS, totalguysRHS, totalhash);
//    fflush(stdout);

    PROFILING(start_time, "<?=$wpName?>", "Merge", "%d\t%d", totalhash, total);

    //output.SwapBitmap (bitmapOut);
    output.SwapBitmap (myOutBStringIter);
/*
    MMappedStorage st;
    Column co (st);
    Bitstring patt(0xf, true);
    BStringIterator iter(co, patt, 0);
    output.SwapBitmap(iter);
*/
    // and give back the result
    ChunkContainer tempResult (output);
    tempResult.swap (result);

    return 0; // have to return something
}
//+{"kind":"WPF", "name":"Merge", "action":"end"}

<?
}
?>
