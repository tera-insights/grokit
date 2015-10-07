<?php
// Copyright 2014 Tera Insights, LLC. All Rights Reserved.

function GTGenerate( $wpName, $queries, $attMap ) {
    GTGenerate_PreProcess($wpName, $queries, $attMap);
    GTGenerate_ProcessChunk($wpName, $queries, $attMap);
}

function GTGenerate_PreProcess($wpName, $queries, $attMap) {
?>
//+{"kind":"WPF", "name":"Pre-Processing", "action":"start"}
extern "C"
int GTPreProcessWorkFunc_<?=$wpName?>
(WorkDescription& workDescription, ExecEngineData& result) {
    GTPreProcessWD myWork;
    myWork.swap(workDescription);

    QueryExitContainer & queries = myWork.get_whichQueryExits();
    QueryToGLASContMap & reqStates = myWork.get_requiredStates();

    QueryToGLAStateMap constStates;

<?  cgDeclareQueryIDs($queries);  ?>

<?  foreach( $queries as $query => $info ) {
        $gt = $info['gt'];
        if( $gt->has_state() ) {
            $state = $gt->state();
            if( $state->configurable() ) {
                $carg = $info['cargs'];
                echo '    // JSON Configuration for query ' . queryName($query) . PHP_EOL;
                $carg->init();
                echo PHP_EOL;
            } // if const state is configurable
        } // if gt has state
    } // foreach query
?>


    FOREACH_TWL(iter, queries) {
<?  foreach( $queries as $query => $info ) { ?>
        if( iter.query == <?=queryName($query)?> ) {
<?
        $gt = $info['gt'];
        $given_states = $info['states'];
        if( $gt->has_state() ) {
            $state = $gt->state();
            $cstArgs = [];

            // If the state is configurable, give it the JSON carg
            if( $state->configurable() ) {
                $carg = $info['cargs'];
                $cstArgs[] = $carg->name();
            }

            if( \count($given_states) > 0 ) {
?>
            FATALIF(!reqStates.IsThere(<?=queryName($query)?>),
                "No required states received for query that declared required states");
            GLAStateContainer& givenStates = reqStates.Find(<?=queryName($query)?>);
            givenStates.MoveToStart();
            GLAPtr reqTemp;
<?
                foreach( $given_states as $gs ) {
                    $cstArgs[] = '*' . $gs->name();
?>
            // Extract state from waypoint [<?=$gs->waypoint()?>]
            <?=$gs->type()?> * <?=$gs->name()?> = nullptr;
            reqTemp.swap(givenStates.Current());
            FATALIF( reqTemp.get_glaType() != <?=$gs->type()->cHash()?>,
                "Got different type than expected for required state of type <?=$gs->type()?>");
            <?=$gs->name()?> = (<?=$gs->type()?>*) reqTemp.get_glaPtr();
            reqTemp.swap(givenStates.Current());
            givenStates.Advance();

<?
                } // foreach given state
            } // if there are external states

            $cstStr = \count($cstArgs) > 0 ? '(' . implode(', ', $cstArgs) . ')' : '';
?>
            <?=$state?> * temp = new <?=$state?><?=$cstStr?>;
            GLAPtr newPtr( <?=$state->cHash()?>, (void *) temp );
            QueryID qryID = iter.query;
            constStates.Insert(qryID, newPtr);
<?
        } // if the gt has a constant state
?>
        } // If this query is query <?=queryName($query)?>.
<?  } // foreach query ?>
    } END_FOREACH

    GTPreProcessRez myRez( constStates );
    myRez.swap(result);

    return WP_PREPROCESSING;
}
//+{"kind":"WPF", "name":"Pre-Processing", "action":"end"}
<?
} // GTGenerate_PreProcess

function GTGenerate_ProcessChunk($wpName, $queries, $attMap) {
    $allPassthrough = [];
    $allSynth = [];
    foreach( $queries as $query => $info ) {
        $pass = $info['pass'];
        foreach( $pass as $attr ) {
            if( ! in_array($attr, $allPassthrough) )
                $allPassthrough[] = $attr;
        }

        $output = $info['output'];
        foreach( $output as $attr ) {
            if( ! in_array($attr, $allSynth) )
                $allSynth[] = $attr;
        }
    }

    $gtVars = [];
?>
//+{"kind":"WPF", "name":"Process Chunk", "action":"start"}
extern "C"
int GTProcessChunkWorkFunc_<?=$wpName?>
(WorkDescription & workDescription, ExecEngineData & result) {
    GTProcessChunkWD myWork;
    myWork.swap(workDescription);

    Chunk & input = myWork.get_chunkToProcess();

    QueryToGLAStateMap & gtStates = myWork.get_gtStates();
    QueryToGLAStateMap & constStates = myWork.get_constStates();

    QueryIDSet queriesToRun = QueryExitsToQueries( myWork.get_whichQueryExits() );
    FATALIF(queriesToRun.IsEmpty(), "Attempted to process chunk with no queries to run.");

<?  cgDeclareQueryIDs($queries); ?>

    QueryIDSet queriesAvailable;
<?  foreach($queries as $query => $info ) { ?>
    queriesAvailable.Union(<?=queryName($query)?>);
<?  } // for each query ?>

    QueryIDSet queriesCovered = queriesToRun;
    queriesCovered.Intersect(queriesAvailable);

    FATALIF( queriesCovered.IsEmpty(), "Queries being run do not overlap queries known by this waypoint" );

    PROFILING2_START;

    // Set up the output chunk
    Chunk output;

    // Defining the GT states needed.
    // For each one we will look for an existing state, if we find none, we
    // create a new state from scratch.

<?  foreach( $queries as $query => $info ) {
        $gt = $info['gt'];
        $gtVar = 'state_' . queryName($query);
        $gtVars[$query] = $gtVar;

        $cstArgs = [];
        if( $gt->configurable() ) {
            echo '    // JSON Configuration for query ' . queryName($query) . PHP_EOL;
            $carg = $info['cargs'];
            $carg->init();
            $cstArgs[] = $carg->name();
            echo PHP_EOL;
        }

        if( $gt->has_state() ) {
            $cstArgs[] = '*constState';
        }

        if( \count($cstArgs) > 0 ) {
            $cstStr = '(' . implode(', ', $cstArgs) . ')';
        } else {
            $cstStr = '';
        }
?>
    <?=$gt?> * <?=$gtVar?> = nullptr;
    if( queriesToRun.Overlaps(<?=queryName($query)?>) ) {
        if( gtStates.IsThere(<?=queryName($query)?>) ) {
            // Extract from container
            GLAPtr tState;
            GLAState& state = gtStates.Find(<?=queryName($query)?>);
            tState.swap(state);

            // Check type
            FATALIF( tState.get_glaType() != <?=$gt->cHash()?>,
                "Got different type than expected for GT of type <?=$gt?>");

            // Extract pointer
            <?=$gtVar?> = (<?=$gt?> *) tState.get_glaPtr();

            // Return container
            tState.swap(state);
        } // if we already have a state available
        else {
            // We don't have a state, create a new one
<?      if( $gt->has_state() ) { ?>

            // Extract the constant state, so we can use it to initialize
            // the GT
            const <?=$gt->state()?> * constState = nullptr;
            GLAPtr constStatePtr;

            // Extract from container
            FATALIF( !constStates.IsThere(<?=queryName($query)?>),
                "No constant state found for query (<?=queryName($query)?>) that requires a constant state");
            GLAState & tempState = constStates.Find(<?=queryName($query)?>);
            constStatePtr.swap(tempState);

            // Check validity
            FATALIF( constStatePtr.get_glaType() != <?=$gt->state()->cHash()?>,
                "Got different type than expected for constant state of type <?=$gt->state()?>");

            // Get pointer
            constState = (const <?=$gt->state()?> *) constStatePtr.get_glaPtr();

            // Put back in container
            constStatePtr.swap(tempState);

<?      } // if GT has a constant state ?>
            // Construct new GT
            <?=$gtVar?> = new <?=$gt?><?=$cstStr?>;

            // Play into container, so we can reuse it.
            GLAPtr newPtr( <?=$gt->cHash()?>, <?=$gtVar?> );
            QueryIDSet qry = <?=queryName($query)?>;
            gtStates.Insert(qry, newPtr);
        } // if we had to create a state
    } // if query <?=queryName($query)?> is running

<?  } // foreach query ?>

    // Prepare output bitstring iterator
    MMappedStorage queries_out_store;
    Column queries_out_col( queries_out_store );
    BStringIterator queries_out(queries_out_col, queriesCovered);

    // Prepare output for all passthrough attributes
<?  cgConstructColumns($allPassthrough, '_out'); ?>

    // Prepare output for synthesized attributes
<?  cgConstructColumns($allSynth, '_out'); ?>

    // Queries Covered by each passthrough attribute
<?  foreach( $allPassthrough as $attr ) {
        $type = $attr->type();
        $nullable = $type->is('nullable');

        $cstr = "";
        if ($nullable)
            $cstr = "(GrokitNull::Value)";
?>
    <?=$type?> <?=$attr->name()?>_out_default<?=$cstr?>;
    QueryIDSet <?=$attr->name()?>_out_queries;
<?      foreach( $queries as $query => $info ) {
            if( in_array($attr, $info['pass']) ) {
?>
    <?=$attr->name()?>_out_queries.Union(<?=queryName($query)?>);
<?
            } // if attribute is part of this query
        } // foreach query ?>
<?  } // foreach passthrough attribute ?>

    // Queries covered by each synthesized attribute
<?  foreach( $allSynth as $attr ) {
        $type = $attr->type();
        $nullable = $type->is('nullable');

        $cstr = "";
        if ($nullable)
            $cstr = "(GrokitNull::Value)";
?>
    <?=$type?> <?=$attr->name()?>_out_default<?=$cstr?>;
    QueryIDSet <?=$attr->name()?>_out_queries;
<?      foreach( $queries as $query => $info ) {
            if( in_array($attr, $info['output']) ) {
?>
    <?=$attr->name()?>_out_queries.Union(<?=queryName($query)?>);
<?
            } // if attribute is part of this query
        } // foreach query ?>
<?  } // foreach passthrough attribute ?>

    // Define constants used in expressions
<?  foreach( $queries as $query =>  $info ) {
        $input = $info['expressions'];
        cgDeclareConstants($input);
    } // foreach query
?>

    // Profiling information
    int64_t numTuplesIn = 0;
    int64_t numTuplesOut = 0;

#ifdef PER_QUERY_PROFILE
<?  foreach( $queries as $query => $info ) { ?>
    int64_t numTuplesIn_<?=queryName($query)?> = 0;
    int64_t numTuplesOut_<?=queryName($query)?> = 0;
<?  } // foreach query ?>
#endif // PER_QUERY_PROFILE

    // Define should_iterate and is_done for each query
<?  $iterVars = [];
    foreach( $queries as $query => $info ) {
        $iterVar = 'should_process_' . queryName($query);
        $iterVars[] = $iterVar;
?>
    bool <?=$iterVar?> = false;
<?  }
    $iterExpr = implode(' || ', $iterVars);
?>

<?  foreach( $queries as $query => $info ) {
        // Call StartChunk() on iterable GTs
        $gt = $info['gt'];
        $gtVar = $gtVars[$query];
        if ($gt->iterable()) {
?>
    if (queriesToRun.Overlaps(<?=queryName($query)?>)) {
        <?=$gtVar?>->StartChunk();
    }
<?      } ?>
<?  } ?>

    do {
<?  cgAccessColumns($attMap, 'input', $wpName); ?>

    // Prepare input bitstring iterator
    BStringIterator queries_in;
    input.SwapBitmap(queries_in);
    
    numTuplesIn = 0;
#ifdef PER_QUERY_PROFILE
<?  foreach( $queries as $query => $info ) { ?>
    numTuplesIn_<?=queryName($query)?> = 0;
<?  } // foreach query ?>
#endif // PER_QUERY_PROFILE

    // Begin processing tuples
    while( !queries_in.AtEndOfColumn() ) {
        numTuplesIn++;
        QueryIDSet qry = queries_in.GetCurrent();
        qry.Intersect(queriesToRun);
        queries_in.Advance();

<?  cgAccessAttributes($attMap); ?>
<?  foreach( $queries as $query => $info ) {
        $gt = $info['gt'];
        $input = $info['expressions'];
        $output = $info['output'];
        $iterVar = 'should_process_' . queryName($query);

        $gtVar = $gtVars[$query];

        $outputVars = [];
        foreach( $output as $attr ) {
            $outputVars[] = $attr->name() . '_out';
        }

        $inputVals = [];
        foreach( $input as $expr ) {
            $inputVals[] = $expr->value();
        }

        // Prepare the output text
        ob_start();
?>
            // Insert synthesized attributes
<?      foreach( $allSynth as $attr ) { ?>
            if( queriesToRun.Overlaps(<?=$attr->name()?>_out_queries) ) {
				<?=$attr->name()?>_out_Column_Out.Insert(<?=$attr->name()?>_out);
            }
			else {
				<?=$attr->name()?>_out_Column_Out.Insert(<?=$attr->name()?>_out_default);
			}
			<?=$attr->name()?>_out_Column_Out.Advance();
<?      } // foreach synthesized attribute ?>

            // Insert passthrough attributes
<?      foreach( $allPassthrough as $attr ) { ?>
            if( queriesToRun.Overlaps(<?=$attr->name()?>_out_queries) ) {
                <?=$attr->name()?>_out_Column_Out.Insert(<?=$attr->name()?>);
            }
			else {
				<?=$attr->name()?>_out_Column_Out.Insert(<?=$attr->name()?>_out_default);
			}
			<?=$attr->name()?>_out_Column_Out.Advance();
<?      } // foreach synthesized attribute ?>

                queries_out.Insert(<?=queryName($query)?>);
                queries_out.Advance();

                numTuplesOut++;
#ifdef PER_QUERY_PROFILE
                numTuplesOut_<?=queryName($query)?>++;
#endif // PER_QUERY_PROFILE
<?
        $outputText = ob_get_clean();
?>
        // Do Query [<?=queryName($query)?>]
        if( <?=$iterVar?> && qry.Overlaps(<?=queryName($query)?>) ) {
#ifdef PER_QUERY_PROFILE
            numTuplesIn_<?=queryName($query)?>++;
#endif // PER_QUERY_PROFILE
<?      cgDeclarePreprocessing($input, 3); ?>

<?      if( $gt->result_type() == 'single' ) { ?>
            if( <?=$gtVar?>->ProcessTuple(<?=implode(', ', $inputVals)?>, <?=implode(', ', $outputVars)?>) ) {
<?=         $outputText ?>

            } // if GT produced output for this tuple
<?      } else { // if gt result type is single ?>
            <?=$gtVar?>->ProcessTuple(<?=implode(', ', $inputVals)?>);

            while( <?=$gtVar?>->GetNextResult(<?=implode(', ', $outputVars)?>) ) {
<?=         $outputText ?>

            } // while GT has output for this tuple
<?      } // if gt result type is multi ?>
        } // if <?=queryName($query)?> active in tuple

<?  } // foreach query ?>
<?  cgAdvanceAttributes($attMap, 2); ?>
    } // while we have tuples to process

<?  foreach( $queries as $query => $info ) {
        $gt = $info['gt'];
        $iterVar = 'should_process_' . queryName($query);

        $gtVar = $gtVars[$query];

        if ($gt->iterable()) { ?>
    if (<?=$iterVar?>) {
        <?=$iterVar?> = <?=$gtVar?>->ShouldIterate();
    }

<?      } else { ?>
    <?=$iterVar?> = false;
<?      } ?>
<?  } //foreach query ?>

    // Put columns back into chunk
<?  cgPutbackColumns($attMap, 'input', $wpName); ?>

    } while(<?=$iterExpr?>);

    // Tell GTs that need to know about the chunk boundary
<?  foreach( $queries as $query => $info ) {
        $gt = $info['gt'];
        $gtVar = $gtVars[$query];
        if( $gt->chunk_boundary() ) {
?>
    <?=$gtVar?>->ChunkBoundary();
<?      } // if GT cares about chunk boundaries  ?>
<?  } // foreach query ?>



    // Finalize the output iterators and put the columns into the output chunk
    queries_out.Done();
    output.SwapBitmap(queries_out);

<?  foreach( $allPassthrough as $attr ) { ?>
    if( queriesToRun.Overlaps(<?=$attr->name()?>_out_queries) ) {
        Column temp;
        <?=$attr->name()?>_out_Column_Out.Done(temp);
        output.SwapColumn(temp, <?=$attr->slot()?>);
    }
<?  } // foreach passthrough attribute ?>

<?  foreach( $allSynth as $attr ) { ?>
    if( queriesToRun.Overlaps(<?=$attr->name()?>_out_queries) ) {
        Column temp;
        <?=$attr->name()?>_out_Column_Out.Done(temp);
        output.SwapColumn(temp, <?=$attr->slot()?>);
    }
<?  } // foreach passthrough attribute ?>

    PROFILING2_END;

    // Profiling
    PCounterList counterList;
    PCounter totalCntIn("tpi", numTuplesIn, "<?=$wpName?>");
    counterList.Append(totalCntIn);
    PCounter totalCntOut("tpo", numTuplesOut, "<?=$wpName?>");
    counterList.Append(totalCntOut);

#ifdef PER_QUERY_PROFILE
<?  foreach( $queries as $query => $info ) { ?>
    if( queriesToRun.Overlaps(<?=queryName($query)?>) ) {
        PCounter cntIn("tpi <?=queryName($query)?>", numTuplesIn_<?=queryName($query)?>, "<?=$wpName?>");
        counterList.Append(cntIn);
        PCounter cntOut("tpo <?=queryName($query)?>", numTuplesOut_<?=queryName($query)?>, "<?=$wpName?>");
        counterList.Append(cntOut);
    }
<?  } // foreach query ?>
#endif // PER_QUERY_PROFILE

    PROFILING2_SET(counterList, "<?=$wpName?>");

    // Return result and exit
    GTProcessChunkRez gtRez(gtStates, output);
    result.swap(gtRez);

    return WP_PROCESS_CHUNK;
}
//+{"kind":"WPF", "name":"Process Chunk", "action":"end"}
<?
} // GTGenerate_ProcessChunk

?>
