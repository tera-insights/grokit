<?

/* Copyright Tera Insights, 2013, All Rights Reserved

*/

function JoinRHS($wpName, $jDesc){

?>

//+{"kind":"WPF", "name":"RHS Hash", "action":"start"}
extern "C"
int JoinRHSWorkFunc_<?=$wpName?> (WorkDescription &workDescription, ExecEngineData &result) {

    double start_time = global_clock.GetTime();
    PROFILING2_START;

    // this is the area where all of the intermediate, serialized records are stored
    SerializedSegmentArray serializedSegments [NUM_SEGS];

    // this is the area where all of the records are serialized to;
    // 10K bytes are initially used for this
    void *serializeHere = (void *) malloc (10000);
    int storageSize = 10000;

    // go to the work description and get the input chunk
    JoinRHSWorkDescription myWork;
    myWork.swap (workDescription);
    Chunk &input = myWork.get_chunkToProcess ();

    // get the waypoint identifier
    unsigned int wayPointID = myWork.get_wayPointID ();

    QueryIDSet queriesToRun = QueryExitsToQueries(myWork.get_whichQueryExits ());

<?  cgAccessColumns($jDesc->attribute_queries_RHS, 'input', $wpName); ?>

    // prepare bitstring iterator
    Column inBitCol;
    BStringIterator queries;
    input.SwapBitmap (queries);

    int totalNum = 0; // counter for the tuples processed

    // now actually hash all of the tuples!
    while (!queries.AtEndOfColumn ()){
        QueryIDSet qry;
        qry = queries.GetCurrent();
        qry.Intersect(queriesToRun);
        queries.Advance();

        // extract values of attributes from streams
<? cgAccessAttributes($jDesc->attribute_queries_RHS); ?>

         if (qry.IsEmpty()){
<?     cgAdvanceAttributes($jDesc->attribute_queries_RHS); ?>
             continue;
         }

        totalNum++;

<? foreach($jDesc->query_classes_hash as $qClass) {
        $attOrder = [];
        foreach( $qClass->att_queries as $att => $qrys ) {
            $attr = lookupAttribute($att);
            $attOrder[$att] = $attr->slot();
        }

        asort($attOrder);
?>
        // Dealing with join attributes <?=implode(",",$qClass->att_list)?>

        if (qry.Overlaps(QueryIDSet(<?=$qClass->qClass?>, true))) {

            HT_INDEX_TYPE hashValue = HASH_INIT;
    <? foreach($qClass->rhs_keys as $att) { ?>
            hashValue = CongruentHash(Hash(<?=$att?>), hashValue);
    <? } /*foreach attribute*/ ?>

            // figure out which of the hash buckets it goes into
            unsigned int index = WHICH_SEGMENT (hashValue);

            // and serialize the record!  Begin with the bitstring.
            // TBD TBD SS: check if Bitstring takes value that way !
            Bitstring myInBString(<?=$qClass->qClass?>, true);
            myInBString.Intersect(qry);

            int bytesUsed = sizeof(Bitstring);

            // Make sure we have the storage...
            if (bytesUsed > storageSize) {
                storageSize = bytesUsed;
                free (serializeHere);
                serializeHere = (void *) malloc (storageSize);
            }

            // do the serialization...
            void *location = (void*)&myInBString;

            // remember the serialized value
            serializedSegments[index].StartNew (WHICH_SLOT (hashValue), wayPointID, 1, location, bytesUsed);

            // now, go thru all of the attributes that are used
<?      foreach($attOrder as $att => $slot) {
            $qrys = $qClass->att_queries->$att;
?>
            if (myInBString.Overlaps(QueryIDSet(<?=$qrys?>, true))){

                bytesUsed = <?=attSerializedSize($att, $att)?>;
                if (bytesUsed > storageSize) {
                    storageSize = bytesUsed;
                    free (serializeHere);
                    serializeHere = (void *) malloc (storageSize);
                }

                // and record the serialized value
                location =  <?=attOptimizedSerialize($att, $att, "serializeHere")?>;
                serializedSegments[index].Append (<?=$slot?>, location, bytesUsed);
            }
    <? } /*foreach attribute*/ ?>
        }

<? } /*foreach query class*/ ?>
<? /* Is this correct. Should it be inside the loop for the class? */ ?>
<?     cgAdvanceAttributes($jDesc->attribute_queries_RHS); ?>
    }

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

<?  cgPutbackColumns($jDesc->attribute_queries_RHS, 'input', $wpName); ?>

    PROFILING2_END;

    PROFILING(start_time, "<?=$wpName?>", "RHS_hash", "%d", totalNum);
    PROFILING(0.0, "HashTable", "fillrate", "%2.4f", HashTableSegment::globalFillRate*100.0);

    // Finish performance counters
    // Use the Set functionality in case we add additional counters later.
    PCounterList counterList;
    PCounter totalCnt("RHS", totalNum, "<?=$wpName?>");
    counterList.Append(totalCnt);
    PCounter globalCnt("jRHS", totalNum, "global");
    counterList.Append(globalCnt);

    PROFILING2_SET(counterList, "<?=$wpName?>");

    int64_t hFillRate = int64_t(HashTableSegment::globalFillRate * 1000);
    PROFILING2_INSTANT("hfr", hFillRate, "global");

    // now we are finally done!
    JoinHashResult myResult (mySamples);
    myResult.swap (result);
    return 0;

} // JoinRHSWorkFunc_<?=$wpName?> function
//+{"kind":"WPF", "name":"RHS Hash", "action":"end"}

<? } /* JoinRHS function  */ ?>
