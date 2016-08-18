<?

/* Copyright Tera Insights, 2013, All Rights Reserved

 */

function JoinLHS($wpName, $jDesc) {
    $rhsAttOrder = [];
    foreach ($jDesc->hash_RHS_attr as $attr) {
        $att = lookupAttribute($attr);
        $rhsAttOrder[$att->slot()] = $attr;
    }

    ksort($rhsAttOrder);

    // Looking up null constructors for the RHS attributes for the outer join.
    // This will implicitly throw an error if no matching constructor is found.
    if ($jDesc->left_target > 0)
        foreach ($jDesc->attribute_queries_RHS as $att => $queries)
            $rhsConstructors[$att] = lookupFunction(lookupType(attType($att)), [lookupType('NULL')]);

    $jDesc->hash_RHS_attr = $rhsAttOrder;
?>

//+{"kind":"WPF", "name":"LHS Lookup", "action":"start"}
extern "C"
int JoinLHSWorkFunc_<?=$wpName?>(WorkDescription &workDescription, ExecEngineData &result) {

  double start_time = global_clock.GetTime();
  PROFILING2_START;
  // this is the area where all of the intermediate, serialized records are stored
  SerializedSegmentArray serializedSegments [NUM_SEGS];

  // this is the area where all of the records are serialized to;
  // 10K bytes are initially used for this
  char *serializeHere = (char *) malloc (10000);

  // this is the output chunk
  Chunk output;

  // go to the work description and get the input chunk
  JoinLHSWorkDescription myWork;
  myWork.swap (workDescription);
  Chunk &input = myWork.get_chunkToProcess ();

  // get the waypoint ID from the chunk
  int wayPointID = myWork.get_wayPointID ();

  QueryIDSet queriesToRun = QueryExitsToQueries(myWork.get_whichQueryExits ());

<?  cgAccessColumns($jDesc->attribute_queries_LHS, 'input', $wpName); ?>

  BStringIterator myInBStringIter;
  input.SwapBitmap (myInBStringIter);

  // start the iterators for the output columns for LHS; used only if stillShallow = 0
<?  foreach ($jDesc->attribute_queries_LHS_copy as $att => $queries){ ?>
  <?=attIteratorType($att)?> <?=$att?>_Column_Out;
<?  } /*foreach*/ ?>

  // these manage the output columns that come from the RHS (now stored in the hash table)
<?  cgConstructColumns(array_keys($jDesc->attribute_queries_RHS_copy));  ?>

  // this is the ouput bitstring
  MMappedStorage myStore;
  Column bitmapOut (myStore);
  BStringIterator myOutBStringIter (bitmapOut, queriesToRun);

  // now we extract all of the hash table segments... after this, myEntries will hold them all
  HashTableView myView;
  myWork.get_centralHashTable ().EnterReader (myView);
  HashTableSegment myEntries[NUM_SEGS];
  myView.ExtractAllSegments (myEntries);

  // this tells us that we are "still shallow"---not making a deep copy of the LHS atts to the output
  int stillShallow = 1;

  // the bitstring that will be exracted from the hash table
  QueryIDSet *bitstringRHS = 0;

  QueryIDSet existsTarget(<?=$jDesc->exists_target?>, true);
  QueryIDSet notExistsTarget(<?=$jDesc->not_exists_target?>, true);
  // Bitstring for queries that are an outer join on the left.
  QueryIDSet leftTarget(<?=$jDesc->left_target?>, true);


  // these are all of the attribute values that come from the hash table...
  // for each att we need a pointer as well as a dummy value that the pointer will be set to by default

<?  foreach ($jDesc->attribute_queries_RHS as $att => $queries) { ?>
  QueryIDSet <?=attQrys($att)?>_RHS(<?=$queries?>, true);
  <?=attType($att)?> <?=$att?>RHSShadow;
  <?=attType($att)?> *<?=$att?>RHS = NULL;
  <?=attType($att)?> <?=$att?>RHSobj;
<?  } /*foreach*/ ?>

  // now actually try to match up all of the tuples!
  int totalNum = 0;
  while (!myInBStringIter.AtEndOfColumn ()) { // TBD, probably this is not working TBD

    // counts how many matches for this query
    int numHits = 0;

    // extract values of attributes from streams
    // now go through the LHS input atts one at a time and extract if it is needed by an active query

    // see which queries match up
    QueryIDSet curBits = myInBStringIter.GetCurrent ();
    curBits.Intersect (queriesToRun);

    QueryIDSet exists; // keeps track of the queries for which a match is found
    QueryIDSet oldBitstringLHS; // last value of bitstringLHS


    // if the input query is not empty
    if (!curBits.IsEmpty ()) {

      totalNum++;

      // compute the hash for LHS
      HT_INDEX_TYPE hashValue = HASH_INIT;
<?  foreach ($jDesc->LHS_keys as $att) { ?>
      hashValue = CongruentHash(Hash(<?=$att?>_Column.GetCurrent()), hashValue);
<?  } /*foreach*/ ?>

      // figure out which of the hash buckets it goes into
      unsigned int index = WHICH_SEGMENT (hashValue);

      // now, go to that index and extract matching tuples!
      HT_INDEX_TYPE curSlot = WHICH_SLOT (hashValue);
      hashValue = curSlot;

      // this loops through all of the possible RHS hits
      while (1) {

        // this is the bitstring that will go in the output
        QueryIDSet bitstringLHS;

        // for safety (in case we look at a bitstring that spans multiple
        // entries that is not done being written by a concurrent writer)
        // empty out the inital bitstring
        ((QueryIDSet *) serializeHere)->Empty ();

        // give safe "shadow" values to all of the RHS attributes
<?  foreach ($jDesc->hash_RHS_attr as $att) { ?>
        <?=$att?>RHS = &<?=$att?>RHSShadow;
<?  } /*foreach*/ ?>

        // here we go through and extract the atts one at a time from the hash
        // table.  Note that the atts must be extracted IN ORDER.  That is, the
        // bitstring is first, followed by the att mapped to the lowerest column
        // position, followed by the att mapped to the next lowest, and so on.

        // The Extract function pulls an attribute out of the hash table...
        int lenSoFar = 0, dummy, done;
        int lastLen = myEntries[index].Extract(serializeHere, curSlot,
                                               hashValue, wayPointID, BITMAP,
                                               dummy, done);

        // if we cannot find a bitstring, there was no tuple here, and we are done
        if (lastLen == 0)
          break;

        // remember the bitstring
        bitstringRHS = (QueryIDSet *) serializeHere;
        lenSoFar += lastLen;

        // next look for other hashed attributes
<?  foreach ($jDesc->hash_RHS_attr as $att) { ?>
        lastLen = myEntries[index].Extract(serializeHere + lenSoFar, curSlot,
                                           hashValue, wayPointID,
                                           <?=attSlot($att)?>, dummy, done);

        // see if we got attribute
        if (lastLen > 0) {
          Deserialize(serializeHere + lenSoFar, <?=$att?>RHSobj);
          //<?=attOptimizedDeserialize($att, $att."RHSobj", "serializeHere", "lenSoFar")?>;
          <?=$att?>RHS = &<?=$att?>RHSobj;
          lenSoFar += lastLen;
        } else {
          FATALIF(<?=attQrys($att)?>_RHS.Overlaps(*bitstringRHS),
                  "Did not find attribute <?=$att?> in active RHS tuple");
        }
<?  } /*foreach*/ ?>

        // see if we have any query matches
        bitstringRHS->Intersect (curBits);
        QueryIDSet qBits;

<?  foreach ($jDesc->queries_attribute_comparison as $qClass) { ?>
        // See if any query in query class is eligible for this comparision
        qBits = QueryIDSet(<?=$qClass->qClass?>, true);
        qBits.Intersect(*bitstringRHS);
        if (!qBits.IsEmpty()
<?      foreach ($qClass->att_pairs as $pair) { ?>
          && *<?=$pair->rhs?>RHS == <?=$pair->lhs?>_Column.GetCurrent()
<?      } ?>) {
          bitstringLHS.Union (qBits);
        }
<?  } /*foreach query class*/ ?>

        // if any of them hit...
        if (!bitstringLHS.IsEmpty ()) {

          exists.Union(bitstringLHS);

          numHits++;

          // see if we need to move from shallow to deep
          if (numHits == 2 && stillShallow) {

<?  foreach ($jDesc->attribute_queries_LHS_copy as $att => $qrys) { ?>
            <?=attData($att)?>_Out.CreateDeepCopy (<?=attData($att)?>);
            <?=attData($att)?>_Out.Insert (<?=attData($att)?>.GetCurrent());
            <?=attData($att)?>_Out.Advance();

<?  } ?>
            stillShallow = 0;
          }

          // now, add all of the outputs over... first deal with the LHS input atts
          // that get copied into output atts
          if (!stillShallow) {
<?  foreach ($jDesc->attribute_queries_LHS_copy as $att => $qrys) { ?>
            <?=attData($att)?>_Out.Insert (<?=attData($att)?>.GetCurrent());
            <?=attData($att)?>_Out.Advance();
<?  } ?>
          }

          // now, deal with the output atts that come from the hash table
<?  foreach ($jDesc->attribute_queries_RHS_copy as $att => $qrys) { ?>
          <?=attData($att)?>_Out.Insert (*<?=$att?>RHS);
          <?=attData($att)?>_Out.Advance();
<?  } ?>


          // finally, set the bitmap. We are one element behind
          if (!oldBitstringLHS.IsEmpty()){
             myOutBStringIter.Insert (oldBitstringLHS);
                 myOutBStringIter.Advance ();
          }
          oldBitstringLHS=bitstringLHS;
        }  // empty bistring
      }
    }

    // compute the true exist queries
    QueryIDSet tmp = existsTarget;
    tmp.Intersect(exists);
    tmp.Intersect(curBits); // not needed but I'm paranoid

    // compute the true not exits queries
    QueryIDSet tmp2 = curBits;
    tmp2.Intersect(notExistsTarget);
    tmp2.Difference(exists);

    // now put everything in bitstringLHS
    oldBitstringLHS.Union(tmp);
    oldBitstringLHS.Union(tmp2);

    // Any query with a left outer join is automatically turned on.
    oldBitstringLHS.Union(leftTarget);

    // There is output for this tuple, so the output bitstring must be advanced.
    if (!oldBitstringLHS.IsEmpty()) {
      myOutBStringIter.Insert(oldBitstringLHS);
      myOutBStringIter.Advance();
    }

    // at this point, we are done trying to join this tuple... any join results have been
    // written to the output columns.  Note that we don't have to advance in the output data
    // columns; if we are shallow, we don't touch the output columns.  If we are not shallow,
    // if there were no results, we have nothing to write.  HOWEVER, if we are shallow and
    // we did not get a match, we need to add an emtpy btstring
    if (numHits == 0) {
      // The RHS columns are advanced as necessary.
      if (stillShallow || leftTarget > 0) {
<?  foreach ($jDesc->attribute_queries_RHS_copy as $att => $qrys) { ?>
<?      if ($jDesc->left_target > 0) { ?>
        // A null object is created using the null constructor.
        <?=attType($att)?> tmp_<?=attData($att)?> = <?=$rhsConstructors[$att]?>(n);
<?      } else { ?>
        // A dummy object is created using the default constructor.
        <?=attType($att)?> tmp_<?=attData($att)?>;
<?      } ?>
        <?=attData($att)?>_Out.Insert (tmp_<?=attData($att)?>);
        <?=attData($att)?>_Out.Advance();
<?  } ?>
      }

      // Copy the LHS input to the output as for a non-shallow left outer join.
      if (leftTarget > 0 && !stillShallow) {
<?  foreach ($jDesc->attribute_queries_LHS_copy as $att => $qrys) { ?>
            <?=attData($att)?>_Out.Insert(<?=attData($att)?>.GetCurrent());
            <?=attData($att)?>_Out.Advance();
<?  } ?>
      }

      // An empty bitstring is inserted for a shallow join that has no output
      // for this tuple. This cannot happen for a left outer join.
      if (stilLShallow && oldBitstringLHS.IsEmpty()) {
        myOutBStringIter.Insert(oldBitstringLHS);
        myOutBStringIter.Advance();
      }
    }

    // The LHS input is advanced.
<?  foreach ($jDesc->attribute_queries_LHS as $att => $qrys) { ?>
    <?=attData($att)?>.Advance();
<?  } ?>

    // The input bitstring is advanced.
    myInBStringIter.Advance ();
  }

  // The join is completed. The output chunk is now constructed.

  // if we are still shallow, put the original data into the output
  if (stillShallow) {
<?  foreach($jDesc->attribute_queries_LHS_copy as $att => $qrys) { ?>
    Column col_<?=$att?>;
    <?=attData($att)?>.Done(col_<?=$att?>);
    output.SwapColumn (col_<?=$att?>, <?=attSlot($att)?>);
<?  } ?>
  } else {
<?  foreach($jDesc->attribute_queries_LHS_copy as $att => $qrys) { ?>
    Column col_<?=$att?>;
    <?=attData($att)?>_Out.Done(col_<?=$att?>);
    output.SwapColumn (col_<?=$att?>, <?=attSlot($att)?>);
<?  } ?>
  }

<?  foreach($jDesc->attribute_queries_RHS_copy as $att => $qrys) { ?>
  Column col_<?=$att?>;
  <?=attData($att)?>_Out.Done(col_<?=$att?>);
  output.SwapColumn (col_<?=$att?>, <?=attSlot($att)?>);
<?  } /*foreach*/ ?>

  // put in the output bitmap
  myOutBStringIter.Done ();
  output.SwapBitmap (myOutBStringIter);

  // and give back the result
  ChunkContainer tempResult (output);
  tempResult.swap (result);

  PROFILING2_END;

  PROFILING(start_time, "<?=$wpName?>", "LHS_lookup", "%d", totalNum);
  PROFILING(0.0, "HashTable", "fillrate", "%2.4f", HashTableSegment::globalFillRate*100.0);

  // Finish performance counters
  // Use the Set functionality in case we add additional counters later.
  PCounterList counterList;
  PCounter totalCnt("tpi lhs", totalNum, "<?=$wpName?>");
  counterList.Append(totalCnt);

  PROFILING2_SET(counterList, "<?=$wpName?>");

  int64_t hFillRate = int64_t(HashTableSegment::globalFillRate * 1000);
  PROFILING2_INSTANT("hfr", hFillRate, "global");

  free (serializeHere);
  return 1;
}
//+{"kind":"WPF", "name":"LHS Lookup", "action":"end"}

<? } /* JoinLHS function */ ?>

