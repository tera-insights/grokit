<?

/* Copyright Tera Insights, 2013, All Rights Reserved

*/

function JoinLHSHash($wpName, $jDesc){
?>

//+{"kind":"WPF", "name":"LHS Hash", "action":"start"}
extern "C"
int JoinLHSHashWorkFunc_<?=$wpName?> (WorkDescription &workDescription, ExecEngineData &result) {
    double start_time = global_clock.GetTime();

    // this is the area where all of the intermediate, serialized records are stored
    SerializedSegmentArray serializedSegments [NUM_SEGS];

    // this is the area where all of the records are serialized to;
    // 10K bytes are initially used for this
    void *serializeHere = (void *) malloc (10000);
    int storageSize = 10000;

    // go to the work description and get the input chunk
    JoinLHSHashWorkDescription myWork;
    myWork.swap (workDescription);
    Chunk &input = myWork.get_chunkToProcess ();

    // get the waypoint identifier
    unsigned int wayPointID = myWork.get_wayPointID ();

    QueryIDSet queriesToRun = QueryExitsToQueries(myWork.get_whichQueryExits ());

<?  cgAccessColumns($jDesc->attribute_queries_LHS, 'input', $wpName); ?>

    Column inBitCol;
    BStringIterator queries;
    input.SwapBitmap (queries);

    int totalNum = 0; // counter for the tuples processed

    // now actually hash all of the tuples!
    while (!queries.AtEndOfColumn ()) {
        QueryIDSet qry;
        qry = queries.GetCurrent();
        qry.Intersect(queriesToRun);
        queries.Advance();

        // extract values of attributes from streams
<? cgAccessAttributes($jDesc->attribute_queries_LHS); ?>

         if (qry.IsEmpty()){
<?     cgAdvanceAttributes($jDesc->attribute_queries_LHS); ?>
             continue;
         }

        totalNum++;

        HT_INDEX_TYPE hashValue = HASH_INIT;
<? foreach($jDesc->LHS_hash as $att) { ?>
        hashValue = CongruentHash(Hash(<?=$att?>), hashValue);
<? } ?>

        // figure out which of the hash buckets it goes into
        unsigned int index = WHICH_SEGMENT (hashValue);

        // and serialize the record!  Begin with the bitstring.
        int bytesUsed = sizeof(Bitstring);

        // Make sure we have the storage...
        if (bytesUsed > storageSize) {
            storageSize = bytesUsed;
            free (serializeHere);
            serializeHere = (void *) malloc (storageSize);
        }

        // do the serialization...
        void *location = (void*)&qry;

        // remember the serialized value
        serializedSegments[index].StartNew (WHICH_SLOT (hashValue), wayPointID, 0, location, bytesUsed);

        // now, go thru all of the attributes that are used
<? foreach($jDesc->LHS_hash as $att) { ?>
        bytesUsed = <?=attSerializedSize($att, $att)?>;
        if (bytesUsed > storageSize) {
            storageSize = bytesUsed;
            free (serializeHere);
            serializeHere = (void *) malloc (storageSize);
        }

        // and record the serialized value
        location =  <?=attOptimizedSerialize($att, $att, "serializeHere")?>;
        serializedSegments[index].Append (<?=attSlot($att)?>, location, bytesUsed);
<? } ?>
<? foreach($jDesc->attribute_queries_LHS_copy as $att=>$query) { ?>
        if (qry.Overlaps(QueryIDSet(<?=queryName($query)?>, true))){

            bytesUsed = <?=attSerializedSize($att, $att)?>;
            if (bytesUsed > storageSize) {
                storageSize = bytesUsed;
                free (serializeHere);
                serializeHere = (void *) malloc (storageSize);
            }

            // and record the serialized value
            location =  <?=attOptimizedSerialize($att, $att, "serializeHere")?>;
            serializedSegments[index].Append (<?=attSlot($att)?>, location, bytesUsed);
        }
<? } ?>

<?     cgAdvanceAttributes($jDesc->attribute_queries_LHS); ?>

    } // for each tuple

    // now we are done serializing the chunk
    free (serializeHere);

    // so actually do the hashing... first set up the list of the guys we want to hash
    int theseAreOK [NUM_SEGS];
    for (int i = 0; i < NUM_SEGS; i++) {
        theseAreOK[i] = 1;
    }

    // this is the set of sample collisions taken from the over-full segments
    HashSegmentSample mySamples;

    // now go through and, one-at-a-time, add the data to each table segment
    for (int i = 0; i < NUM_SEGS; i++) {

        // first get a segment to add data to
        HashTableSegment checkedOutCopy;
        int whichOne = myWork.get_centralHashTable ().CheckOutOne (theseAreOK, checkedOutCopy);
        theseAreOK[whichOne] = 0;

        // now add the data
        HashSegmentSample mySample;
        if (checkedOutCopy.Insert (serializedSegments[whichOne], mySample)) {

            // if we are in here, it means that the segment was over-full, so note that we will
            // need to empty it out... we record all of the samples
            mySamples.MoveToFinish ();
            mySample.MoveToStart ();
            mySamples.SwapRights (mySample);
        }

        // and then put the segment back in the hash table
        myWork.get_centralHashTable ().CheckIn (whichOne);
    }

<?  cgPutbackColumns($jDesc->attribute_queries_LHS, 'input', $wpName); ?>

    PROFILING(start_time, "<?=$wpName?>", "LHS_hash", "%d", totalNum);
    PROFILING(0.0, "HashTable", "fillrate", "%2.4f", HashTableSegment::globalFillRate.load());

    // now we are finally done!
    JoinHashResult myResult (mySamples);
    myResult.swap (result);
    return 0;
}
//+{"kind":"WPF", "name":"LHS Hash", "action":"end"}

<?
}
?>
