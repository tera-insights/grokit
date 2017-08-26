<?php

// Copyright 2013 Tera Insights, LLC. All Rights Reserved.

function GLAGenerate( $wpName, $queries, $attMap ) {
    //echo PHP_EOL . '/*' . PHP_EOL;
    //print_r($wpName);
    //print_r($queries);
    //print_r($attMap);
    //echo PHP_EOL . '*/' . PHP_EOL;

    GLAGenerate_PreProcess( $wpName, $queries, $attMap );
    GLAGenerate_ProcessChunk( $wpName, $queries, $attMap );
    GLAGenerate_MergeStates( $wpName, $queries, $attMap );
    GLAGenerate_PreFinalize( $wpName, $queries, $attMap );
    GLAGenerate_Finalize( $wpName, $queries, $attMap );
    GLAGenerate_FinalizeState( $wpName, $queries, $attMap );
    GLAGenerate_PostFinalize( $wpName, $queries, $attMap );
} // end function GLAGenerate

function GLAGenerate_PreProcess( $wpName, $queries, $attMap ) {

?>
// module specific headers to allow separate compilation
#include <iomanip>
#include <assert.h>
#include "Errors.h"

//+{"kind":"WPF", "name":"Pre-Processing", "action":"start"}
extern "C"
int GLAPreProcessWorkFunc_<?=$wpName?>
(WorkDescription& workDescription, ExecEngineData& result) {

    GLAPreProcessWD myWork;
    myWork.swap(workDescription);

    QueryExitContainer& queries = myWork.get_whichQueryExits();

    QueryToGLASContMap & reqStates = myWork.get_requiredStates();
    QueryToGLAStateMap constStates;
    QueryIDSet produceIntermediates;

<?
    cgDeclareQueryIDs($queries);
?>

<?  foreach( $queries as $query => $info ) {
        $gla = $info['gla'];
        if( $gla->has_state() ) {
            $state = $gla->state();
            if( $state->configurable() ) {
                $carg = $info['cargs'];
                echo '    // JSON Configuration for query ' . queryName($query) . PHP_EOL;
                $carg->init();
                echo PHP_EOL;
            } // if gla const state is configurable
        } // if gla has state
    } //foreach query
?>

    FOREACH_TWL(iter, queries) {
<?  foreach( $queries as $query => $info ) { ?>
        if( iter.query == <?=queryName($query)?> ) {
<?
        $gla = $info['gla'];
        $given_states = $info['states'];
        if( $gla->has_state() ) {
            $state = $gla->state();
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
                    $cstArgs[] = '*' . $gs->name();
                } // foreach given state
            } // if there are external states

            $cstStr = \count($cstArgs) > 0 ? '(' . implode(', ', $cstArgs) . ')' : '';
?>
            <?=$state?> * temp = new <?=$state?><?=$cstStr?>;
            GLAPtr newPtr( <?=$state->cHash()?>, (void*) temp );
            QueryID qryID = iter.query;
            constStates.Insert(qryID, newPtr);
<?      } // if gla has constant state ?>

<?      if( $gla->iterable() && $gla->intermediates() ) { ?>
            produceIntermediates.Union(<?=queryName($query)?>);
<?  } // if GLA produces intermediate results ?>
        } // If this query is query <?=queryName($query)?>.
<?  } // foreach query ?>
    } END_FOREACH;

    GLAPreProcessRez myRez( constStates, produceIntermediates );
    myRez.swap(result);

    return WP_PREPROCESSING; // for PreProcess
}
//+{"kind":"WPF", "name":"Pre-Processing", "action":"end"}

<?
} // end function GLAGenerate_PreProcess

function GLAGenerate_MergeStates( $wpName, $queries, $attMap ) {
?>
//+{"kind":"WPF", "name":"Merge States", "action":"start"}
extern "C"
int GLAMergeStatesWorkFunc_<?=$wpName?>
(WorkDescription &workDescription, ExecEngineData &result) {
    GLAMergeStatesWD myWork;
    myWork.swap(workDescription);

    QueryToGLASContMap& queryGLACont = myWork.get_glaStates();
    QueryExitContainer& queries = myWork.get_whichQueryExits();

<?
    cgDeclareQueryIDs($queries);
?>

    QueryToGLAStateMap resultQueryGLASt;
    FOREACH_TWL(iter, queries){
        FATALIF(!queryGLACont.IsThere(iter.query), "Why did we get a query in the list but no stateContainer for it");
        GLAStateContainer& glaContainer = queryGLACont.Find(iter.query);
        GLAPtr mainState;

<?
    foreach( $queries as $query => $info ) {
        $gla = $info['gla'];
        $glaHashVal = $gla->cHash();
?>
        if( iter.query == <?=queryName($query)?> ) {
            // extract first element and convert
            FATALIF(mainState.IsValid(), "Why is the state valid? Did more than 1 query fie up?");
            FATALIF(glaContainer.Length()==0, "There should be at least one state in the list");
            glaContainer.Remove(mainState); //grab first state
            FATALIF( mainState.get_glaType() != <?=$glaHashVal?>, "Got a GLA of a different type!");
            <?=$gla?> * mainGLA = (<?=$gla?> *) mainState.get_glaPtr();

            // scan remaining elements, convert and call AddState
            FOREACH_TWL(g, glaContainer) {
                GLAPtr localState;
                localState.swap(g);
                FATALIF( localState.get_glaType() != <?=$glaHashVal?>, "Got a GLA of a different type!");

                <?=$gla?> * localGLA = (<?=$gla?> *) localState.get_glaPtr();
                mainGLA->AddState(*localGLA);

                // localGLA eaten up. delete
                delete localGLA;
            } END_FOREACH
        }
<?
    } // foreach query
?>
        resultQueryGLASt.Insert(iter.query, mainState);
    } END_FOREACH;

    GLAStatesRez rez(resultQueryGLASt);
    rez.swap(result);

    return WP_POST_PROCESSING; // for merge
}
//+{"kind":"WPF", "name":"Merge States", "action":"end"}

<?
} // end function GLAGenerate_MergeStates

function GLAGenerate_PreFinalize( $wpName, $queries, $attMap ) {
?>
//+{"kind":"WPF", "name":"Pre-Finalize", "action":"start"}
extern "C"
int GLAPreFinalizeWorkFunc_<?=$wpName?>
(WorkDescription &workDescription, ExecEngineData &result) {
    GLAPreFinalizeWD myWork;
    myWork.swap(workDescription);

    QueryToGLAStateMap& queryGLAStates = myWork.get_glaStates();
    QueryToGLAStateMap& queryConstStates = myWork.get_constStates();
    QueryExitContainer& queries = myWork.get_whichQueryExits();

    QueryIDToInt fragments; // fragments for the GLAs that need it
    QueryIDSet iterateMap; // Whether or not each GLA needs to iterate after producing output (if any)

<?
    cgDeclareQueryIDs($queries);
?>

    FOREACH_TWL(iter, queries) {
        FATALIF(!queryGLAStates.IsThere(iter.query), "Why did we get a query in the list but no stateContainer for it");
        GLAState& curState = queryGLAStates.Find(iter.query);

<?
    foreach( $queries as $query => $info ) {
        $gla = $info['gla'];
        $glaHashVal = $gla->cHash();
        $resType = $gla->result_type();
        $fragmented = in_array('fragment', $resType);

        $cstArgs = [];
        if( $gla->configurable() ) {
            $carg = $info['cargs'];
            $cstArgs[] = $carg->name();
        }

        if( $gla->has_state() ) {
            $cstArgs[] = '*constState';
        }

        if( \count($cstArgs) > 0 ) {
            $cArgs = '(' . implode(', ', $cstArgs) . ')';
        } else {
            $cArgs = '';
        }
?>
        if( iter.query == <?=queryName($query)?> ) {
<?      if( $gla->has_state() ): ?>
            // Extract constant state from container
            FATALIF( ! queryConstStates.IsThere(iter.query),
                "Why did we get no constant state for an iterable GLA?");

            GLAState & myStateContainer = queryConstStates.Find( iter.query );
            GLAPtr myStatePtr;
            myStatePtr.swap(myStateContainer);

            // Ensure it's the proper type
            FATALIF( myStatePtr.get_glaType() != <?=$gla->state()->cHash()?>,
                "Constant state for query <?=queryName($query)?> had invalid type");

            // Get the pointer to the state
            <?=$gla->state()?> * constState = (<?=$gla->state()?> *) myStatePtr.get_glaPtr();

            // Pack back into container
            myStatePtr.swap(myStateContainer);

<?      endif; // gla has state ?>
            GLAPtr localState;
            localState.swap(curState);

            <?=$gla?> * localGLA = nullptr;

            if( localState.IsValid() ) {
                FATALIF( localState.get_glaType() != <?=$glaHashVal?>, "Got a GLA of a different type!");
                localGLA = (<?=$gla?> *) localState.get_glaPtr();
            } else {
<?      if( $gla->configurable() ) {
            // Initialize the JSON argument if required
            $carg->init();
        }
?>
                localGLA = new <?=$gla?><?=$cArgs?>;
                GLAPtr temp(<?=$gla->cHash()?>, localGLA);
                localState.swap(temp);
            }

            localState.swap(curState);

            bool iterateRet = false;
<?      if( $gla->iterable() ) { ?>
            // Determine whether this query should iterate
            iterateRet = localGLA->ShouldIterate( *constState );
            if( iterateRet )
                iterateMap.Union(iter.query);
<?      } // if GLA is iterable  ?>

            // Handle fragments
            QueryID foo = <?=queryName($query)?>;
            Swapify<int> val(1);
<?
        if( $fragmented ) {
            if (!$gla->intermediates()) {
?>
            if (iterateRet)
                val = 0;
            else
<?          } ?>
                val = localGLA->GetNumFragments();
<?
        }  // if $fragmented
?>
            fragments.Insert(foo, val);
        } // if current query is query <?=queryName($query)?>
<?  } // foreach query ?>

    } END_FOREACH

    GLAStatesFrRez rez(queryGLAStates, queryConstStates, fragments, iterateMap);
    rez.swap(result);

    return WP_PRE_FINALIZE; // for PreFinalize
}
//+{"kind":"WPF", "name":"Pre-Finalize", "action":"end"}
<?
} // end function GLAGenerate_PreFinalize

function GLAGenerate_Finalize( $wpName, $queries, $attMap ) {
?>
#ifndef PER_QUERY_PROFILE
#define PER_QUERY_PROFILE
#endif

//+{"kind":"WPF", "name":"Finalize (Chunk)", "action":"start"}
extern "C"
int GLAFinalizeWorkFunc_<?=$wpName?>
(WorkDescription &workDescription, ExecEngineData &result) {
    GLAFinalizeWD myWork;
    myWork.swap (workDescription);
    QueryExit whichOne = myWork.get_whichQueryExit();
    GLAState& glaState = myWork.get_glaState();
<?
    cgDeclareQueryIDs($queries);
?>

    // Set up the output chunk
    Chunk output;

    QueryIDSet queriesToRun = whichOne.query;
<?
    // Extract the state for the query
    foreach( $queries as $query => $info ) {
        $gla = $info['gla'];
?>
    // Do query <?=queryName($query)?>:
    <?=$gla?> * state_<?=queryName($query)?> = NULL;
    if( whichOne.query == <?=queryName($query)?> ) {
        // Look for the state of query <?=queryName($query)?>.
        GLAPtr state;
        state.swap(glaState);
        FATALIF( state.get_glaType() != <?=$gla->cHash()?>,
            "Got GLA of unexpected type");
        state_<?=queryName($query)?> = (<?=$gla?> *) state.get_glaPtr();
    }
<?
    } // foreach query
?>

    // Start columns for all possible outputs.
<?
    foreach( $queries as $query => $info ) {
        $output = $info['output'];

        cgConstructColumns($output);
    } // foreach query
?>

    // This is the output bitstring
    MMappedStorage myStore;
    Column bitmapOut( myStore );
    BStringIterator myOutBStringIter( bitmapOut, queriesToRun );

    PROFILING2_START;
    int64_t numTuples = 0;

#ifdef PER_QUERY_PROFILE
<?  foreach( $queries as $query => $info ) { ?>
    int64_t numTuples_<?=queryName($query)?> = 0;
<?  } // foreach query ?>
#endif // PER_QUERY_PROFILE

    // Extract results
<?
    foreach( $queries as $query => $info ) {
        $gla = $info['gla'];
        $output = $info['output'];

        // If this is true, we return the GLA as a const state.
        // Otherwise, we pack the results into a chunk.
        $retState = $info['retState'];

        $stateName = 'state_' . queryName($query);
?>
    if( whichOne.query == <?=queryName($query)?> ) {
<?
        if( $retState ) {
?>
        FATAL( "Called normal finalize for query that was supposed to be returned as a const state" );
<?
        } // if we should return the GLA as a const state
        else {
            $resType = $gla->result_type();
            $resType = get_first_value($resType, [ 'fragment', 'multi', 'single', 'state' ] );
            if( $resType == 'single' ) {
?>
        <?=$stateName?>->GetResult(<?=implode(', ', $output)?>);
        numTuples++;
#ifdef PER_QUERY_PROFILE
        numTuples_<?=queryName($query)?>++;
#endif // PER_QUERY_PROFILE
        myOutBStringIter.Insert(<?=queryName($query)?>);
        myOutBStringIter.Advance();
<?
                cgInsertAttributesList($output, '_Column_Out', 2);
            } elseif( $resType == 'multi' ) {
?>
        <?=$stateName?>->Finalize();
        while( <?=$stateName?>->GetNextResult(<?=implode(', ', $output)?>) ) {
            numTuples++;
#ifdef PER_QUERY_PROFILE
            numTuples_<?=queryName($query)?>++;
#endif // PER_QUERY_PROFILE
            myOutBStringIter.Insert(<?=queryName($query)?>);
            myOutBStringIter.Advance();
<?
                cgInsertAttributesList($output, '_Column_Out', 3);
?>
        }
<?
            } elseif( $resType == 'fragment' ) {
?>
        int fragment = myWork.get_fragmentNo();
        <?=$gla?>_Iterator * iterator = <?=$stateName?>->Finalize(fragment);
        while( <?=$stateName?>->GetNextResult(iterator, <?=implode(', ', $output)?>) ) {
            numTuples++;
#ifdef PER_QUERY_PROFILE
            numTuples_<?=queryName($query)?>++;
#endif // PER_QUERY_PROFILE
            myOutBStringIter.Insert(<?=queryName($query)?>);
            myOutBStringIter.Advance();
<?
                cgInsertAttributesList($output, '_Column_Out', 3);
?>
        }
        delete iterator;
<?
            } elseif( $resType == 'state' ) {
                reset($output);
                $att = current($output); // Output attribute
                if( $gla->finalize_as_state() ) {
?>
        <?=$stateName?>->FinalizeState();
<?
                } // if GLA finalized as state
?>
        <?=$att?> = <?=$att->type()?>( <?=$stateName?> );
        numTuples++;
#ifdef PER_QUERY_PROFILE
        numTuples_<?=queryName($query)?>++;
#endif // PER_QUERY_PROFILE
        myOutBStringIter.Insert(<?=queryName($query)?>);
        myOutBStringIter.Advance();
<?
                cgInsertAttributesList($output, '_Column_Out', 2);
            } else {
                grokit_error('GLA ' . $gla . ' has no known result type: [' . implode(',', $resType) . ']' );
            } // switch GLA result type
        } // else GLA produces into a chunk
?>
        myOutBStringIter.Done();
        output.SwapBitmap(myOutBStringIter);

        // Write columns
<?
        foreach( $output as $att ) {
?>
        Column col_<?=$att?>;
        <?=$att?>_Column_Out.Done(col_<?=$att?>);
        output.SwapColumn( col_<?=$att?>, <?=$att->slot()?> );
<?
        } // foreach output attribute
?>
    }
<?
    } // foreach query
?>

    PROFILING2_END;

    PCounterList counterList;
    PCounter totalCnt("tpo", numTuples, "<?=$wpName?>");
    counterList.Append(totalCnt);

#ifdef PER_QUERY_PROFILE
<?  foreach( $queries as $query => $info ) { ?>
    {
        PCounter qCount("tpo <?=queryName($query)?>", numTuples_<?=queryName($query)?>, "<?=$wpName?>");
        counterList.Append(qCount);
    }
<?  } // foreach query ?>
#endif // PER_QUERY_PROFILE

    PROFILING2_SET(counterList, "<?=$wpName?>");

    ChunkContainer tempResult(output);
    tempResult.swap(result);
    return WP_FINALIZE;
}
//+{"kind":"WPF", "name":"Finalize (Chunk)", "action":"end"}
<?
} // end function GLAGenerate_Finalize

function GLAGenerate_FinalizeState( $wpName, $queries, $attMap ) {
?>
//+{"kind":"WPF", "name":"Finalize (State)", "action":"start"}
extern "C"
int GLAFinalizeStateWorkFunc_<?=$wpName?>
(WorkDescription &workDescription, ExecEngineData &result) {
    GLAFinalizeWD myWork;
    myWork.swap (workDescription);
    QueryExit& whichOne = myWork.get_whichQueryExit();
    GLAState& glaState = myWork.get_glaState();

<?
    cgDeclareQueryIDs($queries);

    foreach( $queries as $query => $info ) {
        $gla = $info['gla'];
        $retState = $info['retState'];
?>
    // Do query <?=queryName($query)?>;
    if( whichOne.query == <?=queryName($query)?> ) {
<?
        if( $retState ) {
            if( $gla->finalize_as_state() ) {
?>
        GLAPtr statePtr;
        statePtr.swap(glaState);
        FATALIF( statePtr.get_glaType() != <?=$gla->cHash()?>,
            "Got GLA of unexpected type");
        <?=$gla?> * state = (<?=$gla?> *) statePtr.get_glaPtr();
        state->FinalizeState();
        statePtr.swap(glaState);
<?
            } // if gla finalized as state
        } // if return as state
        else {
?>
        FATAL("Tried to return a GLA as state that shouldn't be!");
<?
        } // else don't return as state
?>
    }
<?
    } // foreach query
?>

    WayPointID myID = WayPointID::GetIdByName("<?=$wpName?>");
    StateContainer stateCont( myID, whichOne, glaState );
    stateCont.swap(result);
    return WP_FINALIZE; // for finalize
}
//+{"kind":"WPF", "name":"Finalize (State)", "action":"end"}
<?
} // end function GLAGenerate_FinalizeState

function GLAGenerate_ProcessChunk( $wpName, $queries, $attMap ) {
?>
#ifndef PER_QUERY_PROFILE
#define PER_QUERY_PROFILE
#endif

#include <map>
#include <sstream>

//+{"kind":"WPF", "name":"Process Chunk", "action":"start"}
extern "C"
int GLAProcessChunkWorkFunc_<?=$wpName?>
(WorkDescription &workDescription, ExecEngineData &result) {

    GLAProcessChunkWD myWork;
    myWork.swap(workDescription);
    Chunk &input = myWork.get_chunkToProcess ();

    QueryToGLAStateMap& glaStates = myWork.get_glaStates();
    QueryToGLAStateMap& constStates = myWork.get_constStates();
    QueryToGLAStateMap& garbageStates = myWork.get_garbageStates();

    PROFILING2_START;

    QueryIDSet queriesToRun = QueryExitsToQueries( myWork.get_whichQueryExits() );
    FATALIF(queriesToRun.IsEmpty(), "Why are we processing a chunk with no queries to run?");

<?  cgDeclareQueryIDs($queries); ?>

    QueryIDSet queriesAvailable;
<? foreach($queries as $query => $info ) { ?>
    queriesAvailable.Union(<?=queryName($query)?>);
<? } // for each query ?>

    QueryIDSet queriesCovered = queriesToRun;
    queriesCovered.Intersect(queriesAvailable);

    FATALIF( queriesCovered.IsEmpty(), "Queries being run do not overlap queries known by this waypoint!" );

<?  cgAccessColumns($attMap, 'input', $wpName); ?>

    // prepare bitstring iterator
    BStringIterator queries;
    input.SwapBitmap (queries);

    // Garbage collect old states, if there are any.
<?
    foreach( $queries as $query => $info ) {
        $gla = $info['gla'];
?>
    if( garbageStates.IsThere( <?=queryName($query)?> ) ) {
        QueryID curID = <?=queryName($query)?>;
        QueryID key;
        GLAState state;
        garbageStates.Remove( curID, key, state );

        GLAPtr curPtr;
        curPtr.swap(state);

        <?=$gla?> * garbage = (<?=$gla?> *) curPtr.get_glaPtr();
        delete garbage;
    }
<?
    } // foreach query
?>

    // Defining the GLA states needed.
    // For each one we will look for an existing state, if we find none, we
    // create a state from scratch.
<?
    $glaVars = [];
    foreach( $queries as $query => $info ) {
        $gla = $info['gla'];
        $glaVar = 'state_' . queryName($query);
        $glaVars[$query] = $glaVar;
        // Create the pointer to the GLA

        $cstArgs = [];
        if( $gla->configurable() ) {
            echo '    // JSON Configuration for query ' . queryName($query) . PHP_EOL;
            $carg = $info['cargs'];
            $carg->init();
            $cstArgs[] = $carg->name();
            echo PHP_EOL;
        }

        if( $gla->has_state() ) {
            $cstArgs[] = '*constState';
        }

        if( \count($cstArgs) > 0 ) {
            $cstStr = '(' . implode(', ', $cstArgs) . ')';
        } else {
            $cstStr = '';
        }
?>
    <?=$gla?> * <?=$glaVar?> = NULL;
        if( queriesToRun.Overlaps(<?=queryName($query)?>) ) {
            if( glaStates.IsThere(<?=queryName($query)?>) ) {
                // We already have a state, just extract it.

                GLAPtr tState;
                GLAState& state = glaStates.Find(<?=queryName($query)?>);
                tState.swap(state);
                FATALIF( tState.get_glaType() != <?=$gla->cHash()?>,
                    "Got different type than expected for GLA of type <?=$gla?>");
                <?=$glaVar?> = (<?=$gla?> *) tState.get_glaPtr();
                tState.swap(state);
            }
            else {
                // We don't have a state, create a new one.
<?      if( $gla->has_state() ) { ?>
                // Extract the constant state, so we can use it to initialize
                // the GLA
                const <?=$gla->state()?> * constState = nullptr;
                GLAPtr constStatePtr;

                // Extract from container
                FATALIF( !constStates.IsThere(<?=queryName($query)?>),
                    "No constant state found for query (<?=queryName($query)?>) that requires a constant state");
                GLAState & tempState = constStates.Find(<?=queryName($query)?>);
                constStatePtr.swap(tempState);

                // Check validity
                FATALIF( constStatePtr.get_glaType() != <?=$gla->state()->cHash()?>,
                    "Got different type than expected for constant state of type <?=$gla->state()?>");

                // Get pointer
                constState = (const <?=$gla->state()?> *) constStatePtr.get_glaPtr();

                // Put back in container
                constStatePtr.swap(tempState);

<?      } // if gla has a constant state ?>
                // Construct new GLA
                <?=$glaVar?> = new <?=$gla?><?=$cstStr?>;

                // Place into container, so we can reuse it.
                GLAPtr newPtr( <?=$gla->cHash()?>, <?=$glaVar?> );
                QueryIDSet qry = <?=queryName($query)?>;
                glaStates.Insert(qry, newPtr);
            } // else don't have a GLA state already
    } // if queriesToRun overlaps <?=queryName($query)?>.
<?
    } // foreach query

    // Tell GLAs that wish to know about the pre-chunk boundary.
    foreach( $queries as $query => $info ) {
        $gla = $info['gla'];
        $glaVar = $glaVars[$query];

        if( $gla->pre_chunk() ) {
?>
    <?=$glaVar?>->PreChunk();
<?
        } // if GLA wishes to know about pre-chunk boundary
    } // foreach query

    // Define constants used in expressions
    foreach( $queries as $query => $info ) {
        $input = $info['expressions'];
        cgDeclareConstants( $input );

        // Per-query profile
?>
#ifdef PER_QUERY_PROFILE
    int64_t numTuples_<?=queryName($query)?> = 0;
#endif // PER_QUERY_PROFILE
<?
    } // foreach query
?>

    int64_t numTuples = 0;
    while( !queries.AtEndOfColumn() ) {
        ++numTuples;
        QueryIDSet qry;
        qry = queries.GetCurrent();
        qry.Intersect(queriesToRun);
        queries.Advance();

<?
    cgAccessAttributes($attMap);
    foreach( $queries as $query => $info ) {
        $gla = $info['gla'];
        $input = $info['expressions'];
        $output = $info['output'];

        $glaVar = $glaVars[$query];
?>
        // Do query <?=queryName($query)?>:
        if( qry.Overlaps(<?=queryName($query)?>) ) {
<?
        // Declare preprocessing variables
        cgDeclarePreprocessing($input, 3);
?>
            <?=$glaVar?>->AddItem( <?=implode(', ', $input);?>);

#ifdef PER_QUERY_PROFILE
            numTuples_<?=queryName($query)?>++;
#endif // PER_QUERY_PROFILE
        } // if query overlaps <?=queryName($query)?>.
<?
    } // foreach query

    cgAdvanceAttributes($attMap, 2);
?>
    } // while not at end of input

<?
    // Tell GLAs that wish to know about the chunk boundary.
    foreach( $queries as $query => $info ) {
        $gla = $info['gla'];
        $glaVar = $glaVars[$query];

        if( $gla->chunk_boundary() ) {
?>
    <?=$glaVar?>->ChunkBoundary();
<?
        } // if GLA wishes to know about chunk boundary
    } // foreach query
?>
    PROFILING2_END;

    PCounterList counterList;
    PCounter totalCnt("tpi", numTuples, "<?=$wpName?>");
    counterList.Append(totalCnt);

#ifdef PER_QUERY_PROFILE
    // Add tuple counters to list
<?
    foreach( $queries as $query => $info ) {
?>
    PCounter cnt("tpi <?=queryName($query)?>", numTuples_<?=queryName($query)?>, "<?=$wpName?>");
    counterList.Append(cnt);
<?
    } //foreach query
?>
#endif // PER_QUERY_PROFILE

    PROFILING2_SET(counterList, "<?=$wpName?>");

<?
    cgPutbackColumns($attMap, 'input', $wpName);
?>
    // Finish bitstring iterator
    queries.Done();
    input.SwapBitmap(queries);

    GLAProcessRez glaResult(glaStates, input);
    result.swap(glaResult);

    return WP_PROCESS_CHUNK; // for processchunk
}
//+{"kind":"WPF", "name":"Process Chunk", "action":"end"}
<?
} // end function GLAGenerate_ProcessChunk
?>
