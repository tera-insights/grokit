<?
// Copyright 2012 Alin Dobra and Christopher Jermaine

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

// http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Function to instantiate a GIST Waypoint.
function GISTGenerate($wpName, $queries, $attMap ) {

    GISTGenerate_PreProcess(    $wpName, $queries, $attMap);
    GISTGenerate_NewRound(      $wpName, $queries, $attMap);
    GISTGenerate_DoSteps(       $wpName, $queries, $attMap);
    GISTGenerate_MergeStates(   $wpName, $queries, $attMap);
    GISTGenerate_ShouldIterate( $wpName, $queries, $attMap);
    GISTGenerate_ProduceResults($wpName, $queries, $attMap);
    GISTGenerate_ProduceState(  $wpName, $queries, $attMap);
}

function GISTGenerate_PreProcess($wpName, $queries, $attMap) {
?>

// module specific headers to allow separate compilation
#include <iomanip>
#include <assert.h>
#include "Errors.h"
#include <vector>
#include <utility>

extern "C"
int GISTPreProcessWorkFunc_<?=$wpName?>
(WorkDescription& workDescription, ExecEngineData& result) {

    GISTPreProcessWD myWork;
    myWork.swap(workDescription);

    QueryExitContainer& queries = myWork.get_whichQueryExits();

    QueryToGLASContMap & reqStates = myWork.get_receivedStates();
    QueryToGLAStateMap constStates;
    QueryToGLAStateMap gists;

<?  cgDeclareQueryIDs($queries); ?>

<?  foreach ($queries as $query => $info) {
        $gist = $info['gist'];
        if ($gist->has_state()) {
            $state = $gla->state();
            if ($state->configurable()) {
                $carg = $info['cargs'];
                echo '    // JSON Configuration for query ' . queryName($query) . PHP_EOL;
                $carg->init();
                echo PHP_EOL;
            } // If gist const state is configurable
        } // if gla has state
    } // foreach query
?>

    FOREACH_TWL(iter, queries) {
<?  foreach ($queries as $query => $info) { ?>
        if (iter.query == <?=queryname($query)?>) {
<?      $gist = $info['gist'];
        $given_states = $info['states'];
        $cstArgs = [];

        // If the state is configurable, give it the JSON carg
        if ($gist->configurable()) {
            $carg = $info['cargs'];
            $cstArgs[] = $carg->name();
        } ?>

<?      if (count($given_states) > 0) { ?>
            GLAStateContainer& givenStates = reqStates.Find(<?=queryName($query)?>);
            givenStates.MoveToStart();
            GLAPtr reqTemp;

<?          foreach( $given_states as $gs ) { ?>
            // Extract state from waypoint [<?=$gs->waypoint()?>]
            <?=$gs->type()?>* <?=$gs->name()?> = nullptr;
            reqTemp.swap(givenStates.Current());
            FATALIF( reqTemp.get_glaType() != <?=$gs->type()->cHash()?>,
                "Got different type than expected for required state of type <?=$gs->type()?>");
            <?=$gs->name()?> = (<?=$gs->type()?>*) reqTemp.get_glaPtr();
            reqTemp.swap(givenStates.Current());
            givenStates.Advance();

<?              $cstArgs[] = '*' . $gs->name();
            } // foreach given state ?>


<?      } // if there are external states ?>

<?          $cstStr = \count($cstArgs) > 0 ? '(' . implode(', ', $cstArgs) . ')' : ''; ?>

            <?=$gist?>* state_<?=queryName($query)?> = new <?=$gist, $cstStr?>;
            // Package up the GIST state put it in the map
            GLAPtr gistPtr(<?=$gist->cHash()?>, (void *) state_<?=queryName($query)?>);
            GLAState gistState;
            gistPtr.swap(gistState);
            QueryID key = iter.query;

            gists.Insert(key, gistState);
        } // If this query is query <?=queryName($query)?>

<?  } // foreach query ?>
    } END_FOREACH;

    GISTPreProcessRez myRez( constStates, gists );
    myRez.swap(result);

    return WP_PREPROCESSING; // for PreProcess
}

<?
}

function GISTGenerate_NewRound($wpName, $queries, $attMap) {
?>

extern "C"
int GISTNewRoundWorkFunc_<?=$wpName?>
(WorkDescription& workDescription, ExecEngineData& result) {
    GISTNewRoundWD myWork;
    myWork.swap(workDescription);

    // Inputs
    QueryExitContainer& queries = myWork.get_whichQueryExits();
    QueryToGLAStateMap& gists = myWork.get_gists();

    // Outputs
    QueryToGistWUContainer workUnits;
    QueryToGLAStateMap globalSchedulers;

<?  cgDeclareQueryIDs($queries); ?>

    FOREACH_TWL(iter, queries) {
        FATALIF(!gists.IsThere(iter.query),
            "Told to start new round for query %s with no GIST state.",
            iter.query.GetStr().c_str());

<?  foreach ($queries as $query => $info) {
        $gist = $info['gist']; ?>
        if( iter.query == <?=queryName($query)?> ) {
            // Unpack GIST state
            GLAState& tState = gists.Find(<?=queryName($query)?>);
            GLAPtr tPtr;
            tPtr.swap(tState);

            FATALIF(tPtr.get_glaType() != <?=$gist->cHash()?>,
                "Received GIST of different type than expected for query <?=queryName($query)?>");

            <?=$gist?>* state_<?=queryName($query)?> = (<?=$gist?>*) tPtr.get_glaPtr();

            tPtr.swap(tState);

            // Get the schedulers and GLAs from the GIST
            typedef std::pair<<?=$gist?>::LocalScheduler*, <?=$gist?>::cGLA*> WorkUnit;
            typedef std::vector< WorkUnit > WUVector;

            WUVector gistWorkUnits;

            // Second parameter is the parallelization hint.
            state_<?=queryName($query)?>->PrepareRound( gistWorkUnits, NUM_EXEC_ENGINE_THREADS * 4 );

            // Pack the work units into our own data structures.
            GistWUContainer myWorkUnits;

            for( WUVector::iterator it = gistWorkUnits.begin(); it != gistWorkUnits.end(); ++it ) {
                <?=$gist?>::LocalScheduler* localScheduler = it->first;
                <?=$gist?>::cGLA* gla = it->second;

                GLAPtr lsPtr(<?=hashName('gist_LS')?>, (void*) localScheduler);
                GLAPtr glaPtr(<?=hashName('gist_cGLA')?>, (void*) gla);
                GLAPtr gistPtr(<?=$gist->cHash()?>, (void*) state_<?=queryName($query)?>);

                GISTWorkUnit workUnit(gistPtr, lsPtr, glaPtr);

                myWorkUnits.Insert(workUnit);
            }

            QueryID key = iter.query;
            workUnits.Insert(key, myWorkUnits);
        }
<?  } // End foreach query ?>
    } END_FOREACH;

    GISTNewRoundRez myResult(workUnits);
    myResult.swap(result);

    return WP_PREPARE_ROUND; // New Round
}

<?
}

function GISTGenerate_DoSteps($wpName, $queries, $attMap) {
?>

extern "C"
int GISTDoStepsWorkFunc_<?=$wpName?>
(WorkDescription& workDescription, ExecEngineData& result) {
    GISTDoStepsWD myWork;
    myWork.swap(workDescription);

    // Inputs
    QueryExitContainer& queries = myWork.get_whichQueryExits();
    QueryToGistWorkUnit& workUnits = myWork.get_workUnits();

    // Outputs
    QueryToGistWorkUnit unfinishedWork;
    QueryToGLAStateMap finishedWork;

    PCounterList counterList;
    int64_t numStepsTotal = 0;
    PROFILING2_START;

<?  cgDeclareQueryIDs($queries); ?>

    FOREACH_TWL(iter, queries) {
        FATALIF(!workUnits.IsThere(iter.query),
            "Did not receive a work unit for query %s.",
            iter.query.GetStr().c_str());

        // Unpack the work
        QueryID key;
        GISTWorkUnit curWork;
        workUnits.Remove(iter.query, key, curWork);
        GLAState& gistState = curWork.get_gist();
        GLAState& lsState = curWork.get_localScheduler();
        GLAState& glaState = curWork.get_gla();

        GLAPtr gistPtr;
        gistPtr.swap(gistState);

        GLAPtr lsPtr;
        lsPtr.swap(lsState);

        GLAPtr glaPtr;
        glaPtr.swap(glaState);

        int64_t numSteps = 0;

        bool workFinished = false;
<?  foreach ($queries as $query => $info) {
        $gist = $info['gist']; ?>
        if( iter.query == <?=queryName($query)?> ) {
            // Extract the correct pointer types.
            FATALIF(gistPtr.get_glaType() != <?=$gist->cHash()?>,
                "Received GIST of incorrect type for query <?=queryName($query)?>");
            <?=$gist?>* state_<?=queryName($query)?> = ( <?=$gist?>* ) gistPtr.get_glaPtr();

            FATALIF(lsPtr.get_glaType() != <?=hashName('gist_LS')?>,
                "Received local scheduler of incorrect type for query <?=queryName($query)?>");
            <?=$gist?>::LocalScheduler* localScheduler = (<?=$gist?>::LocalScheduler*) lsPtr.get_glaPtr();

            FATALIF(glaPtr.get_glaType() != <?=hashName('gist_cGLA')?>,
                "Received GLA of incorrect type for query <?=queryName($query)?>");
            <?=$gist?>::cGLA* gla = (<?=$gist?>::cGLA*) glaPtr.get_glaPtr();

            // Do the actual work.
            // In the future we may enhance this to allow for timeouts.
            <?=$gist?>::Task task;
            while( localScheduler->GetNextTask( task ) ) {
                state_<?=queryName($query)?>->DoStep( task, *gla );
                ++numSteps;
            }

            // Deallocate scheduler
            delete localScheduler;
            workFinished = true;

            numStepsTotal += numSteps;

#ifdef PER_QUERY_PROFILE
            PCounter cnt("<?=queryName($query)?>", numSteps, "<?=$wpName?>");
            counterList.Append(cnt);
#endif // PER_QUERY_PROFILE
        }
<?  } // end foreach query ?>

        if( workFinished ) {
            GLAState finishedGLA;
            glaPtr.swap(finishedGLA);

            QueryID key = iter.query;
            finishedWork.Insert(key, finishedGLA);
        }
        else {
            // Need to put the pointers back into the work unit.
            gistPtr.swap(gistState);
            lsPtr.swap(lsState);
            glaPtr.swap(glaState);

            // Insert the work unit into unfinishedWork
            QueryID key = iter.query;
            unfinishedWork.Insert(key, curWork);
        }
    } END_FOREACH;

    PROFILING2_END;

    PCounter totalCnt("total", numStepsTotal, "<?=$wpName?>");
    counterList.Append( totalCnt );
    PCounter globalCnt("GIST", numStepsTotal, "global");
    counterList.Append( globalCnt );

    PROFILING2_SET(counterList, "<?=$wpName?>");

    GISTDoStepRez myResult(unfinishedWork, finishedWork);
    myResult.swap(result);

    return WP_PROCESSING; // DoStep
}

<?
}

function GISTGenerate_MergeStates($wpName, $queries, $attMap) {
?>

extern "C"
int GISTMergeStatesWorkFunc_<?=$wpName?>
(WorkDescription& workDescription, ExecEngineData &result) {
    GISTMergeStatesWD myWork;
    myWork.swap(workDescription);

    // Inputs
    QueryToGLASContMap& toMerge = myWork.get_glaStates();
    QueryExitContainer& queries = myWork.get_whichQueryExits();

    // Outputs
    QueryToGLAStateMap glaStates;

<?  cgDeclareQueryIDs($queries); ?>

    FOREACH_TWL(iter, queries) {
        FATALIF(!toMerge.IsThere(iter.query),
            "Did not receive any states to merge for query %s",
            iter.query.GetStr().c_str());

        GLAStateContainer& glaContainer = toMerge.Find(iter.query);

        FATALIF(glaContainer.Length() == 0,
            "Received a container with no states for query %s",
            iter.query.GetStr().c_str())

        glaContainer.MoveToStart();
        GLAState mainState;
        glaContainer.Remove(mainState);
        GLAPtr mainPtr;
        mainPtr.swap(mainState);

<?  foreach ($queries as $query => $info) {
        $gist = $info['gist']; ?>
        if( iter.query == <?=queryName($query)?> ) {
            FATALIF(mainPtr.get_glaType() != <?=hashName('gist_cGLA')?>,
                "Received a GLA for query <?=queryName($query)?> of an unexpected type.");

            <?=$gist?>::cGLA* mainGLA = (<?=$gist?>::cGLA*) mainPtr.get_glaPtr();

            // Scan the remaining elements and merge them with the main one.
            FOREACH_TWL(g, glaContainer) {
                GLAPtr localState;
                localState.swap(g);

                FATALIF(localState.get_glaType() != <?=hashName('gist_cGLA')?>,
                    "Received a GLA for query <?=queryName($query)?> of an unexpected type.");

                <?=$gist?>::cGLA* localGLA = (<?=$gist?>::cGLA*) localState.get_glaPtr();

                mainGLA->AddState(*localGLA);

                delete localGLA;
            } END_FOREACH;
        }
<?  } // end foreach query ?>

        QueryID key = iter.query;
        glaStates.Insert(key, mainPtr);
    } END_FOREACH;

    GLAStatesRez myResult(glaStates);
    myResult.swap(result);

    return WP_POST_PROCESSING; // Merge States
}

<?
}

function GISTGenerate_ShouldIterate($wpName, $queries, $attMap) {
?>

extern "C"
int GISTShouldIterateWorkFunc_<?=$wpName?>
(WorkDescription &workDescription, ExecEngineData &result) {
    GISTShouldIterateWD myWork;
    myWork.swap(workDescription);

    // Inputs
    QueryExitContainer& queries = myWork.get_whichQueryExits();
    QueryToGLAStateMap& glaStates = myWork.get_glaStates();
    QueryToGLAStateMap& gists = myWork.get_gists();

    // Outputs
    QueryIDToInt fragInfo;
    QueryIDSet queriesToIterate;

<?  cgDeclareQueryIDs($queries); ?>

    FOREACH_TWL(iter, queries) {
        FATALIF(!gists.IsThere(iter.query),
            "Did not receive a GIST for query %s",
            iter.query.GetStr().c_str());

        GLAState& curGist = gists.Find(iter.query);

        GLAPtr curGistPtr;
        curGistPtr.swap(curGist);

<?  foreach ($queries as $query => $info) {
        $gist = $info['gist'];
        $resType = $gist->result_type();
        $fragmented = in_array('fragment', $resType); ?>
        if( iter.query == <?=queryName($query)?> ) {
            FATALIF( curGistPtr.get_glaType() != <?=$gist->cHash()?>,
                "Received a GIST of an unexpected type for query <?=queryName($query)?>");

            <?=$gist?>* state_<?=queryName($query)?> = (<?=$gist?>*) curGistPtr.get_glaPtr();

            // If a GIST produced no work units for this round, we might not have
            // a GLA to tell us if we should iterate. If we don't, we will assume
            // that there will be no iteration.
            if( glaStates.IsThere(<?=queryName($query)?>) ) {
                GLAState& curState = glaStates.Find(<?=queryName($query)?>);
                GLAPtr curStatePtr;
                curStatePtr.swap(curState);

                FATALIF( curStatePtr.get_glaType() != <?=hashName('gist_cGLA')?>,
                    "Received a GLA of an unexpected type for query <?=queryName($query)?>");
                <?=$gist?>::cGLA* gla = (<?=$gist?>::cGLA*) curStatePtr.get_glaPtr();
                if( gla->ShouldIterate() ) {
                    queriesToIterate.Union(<?=queryName($query)?>);
                }

                // GLA is no longer needed, deallocate it.
                delete gla;
            }

            QueryID key = <?=queryName($query)?>;
            int numFrags = 1;

<?      if ($fragmented) { ?>
            numFrags = state_<?=queryName($query)?>->GetNumFragments();
<?      } ?>

            Swapify<int> valFrag(numFrags);
            fragInfo.Insert(key, valFrag);
        }
<?  } // end foreach query ?>
    } END_FOREACH;

    GISTShouldIterateRez myResult(fragInfo, queriesToIterate);
    myResult.swap(result);

    return WP_PRE_FINALIZE; // ShouldIterate
}

<?
}

function GISTGenerate_ProduceResults($wpName, $queries, $attMap) {
?>

extern "C"
int GISTProduceResultsWorkFunc_<?=$wpName?>
(WorkDescription &workDescription, ExecEngineData &result) {
    GISTProduceResultsWD myWork;
    myWork.swap(workDescription);

    // Inputs
    QueryExit& whichOne = myWork.get_whichOne();
    GLAState& gist = myWork.get_gist();

    int fragmentNo = myWork.get_fragmentNo();

    // Outputs
    Chunk output;

<?  cgDeclareQueryIDs($queries); ?>

    QueryIDSet queriesToRun = whichOne.query;

    // Start columns for outputs
<?
    foreach( $queries as $query => $info ) {
        $output = $info['output'];

        cgConstructColumns($output);
    } // foreach query
?>

    // Output bitstring
    MMappedStorage myStore;
    Column bitmapOut(myStore);
    BStringIterator myOutBStringIter (bitmapOut, queriesToRun);

    PROFILING2_START;
    int64_t numTuples = 0;

#ifdef PER_QUERY_PROFILE
<?  foreach( $queries as $query => $info ) { ?>
    int64_t numTuples_<?=queryName($query)?> = 0;
<?  } // foreach query ?>
#endif // PER_QUERY_PROFILE

<?  foreach ($queries as $query => $info) {
        $gist = $info['gist'];
        $output = $info['output'];
        $resType = $gist->result_type();
        $resType = get_first_value($resType, ['fragment', 'multi', 'single', 'state']); ?>
    if( whichOne.query == <?=queryName($query)?> ) {

        // Extract the GIST state
        GLAPtr gistPtr;
        gistPtr.swap(gist);

        FATALIF(gistPtr.get_glaType() != <?=$gist->cHash()?>,
            "GIST producing results is of incorrect type for query <?=queryName($query)?>");

        <?=$gist?>* state_<?=queryName($query)?> = (<?=$gist?>*) gistPtr.get_glaPtr();

<?      switch ($resType) {
            case 'single': { ?>
        {
            state_<?=queryName($query)?>->GetResult(<?=implode(', ', $output)?>);
<?              break; }
            case 'multi': ?>
        state_<?=queryName($query)?>->Finalize();
        while (state_<?=queryName($query)?>->(<?=implode(', ', $output)?>)) {
<?              break;
            case 'fragment': ?>
        <?=$gist?>::Iterator* iterator
            = state_<?=queryName($query)?>->Finalize( fragmentNo );
        while( state_<?=queryName($query)?>->GetNextResult( iterator, <?=implode(', ', $output)?>) ) {
<?              break;
            case 'state':
                reset($output);
                $att = current($output); // Output attribute ?>
        {
<?              if ($gist->finalize_as_state()) { ?>
            state_<?=queryName($query)?>->FinalizeState();
<?              } ?>
             <?=$att?> = <?=$att->type()?>( state_<?=queryName($query)?> );
<?              break;
            default:
                grokit_error("Do not know how to deal with output type of GLA {$gist}::cGla [$resType]");
        } // matches switch ?>

            numTuples++;
#ifdef PER_QUERY_PROFILE
            numTuples_<?=queryName($query)?>++;
#endif // PER_QUERY_PROFILE

            // Advance the columns
            myOutBStringIter.Insert(<?=queryName($query)?>);
            myOutBStringIter.Advance();

<?      foreach ($output as $att) { ?>
            <?=$att?>_Column_Out.Insert(<?=$att?>);
            <?=$att?>_Column_Out.Advance();
<?      } ?>
        } // Matches block for column stuff.
<?      if ($resType == 'fragment') { ?>
        // Delete the iterator;
        delete iterator;
<?      } ?>
    } // Matches whichOne
<?  } // matches foreach query ?>

    myOutBStringIter.Done();
    output.SwapBitmap(myOutBStringIter);

    // Write columns
<?  foreach ($queries as $query => $info) {
        $gist = $info['gist'];
        $output = $info['output']; ?>
    if( whichOne.query == <?=queryName($query)?> ) {
<?      foreach ($output as $att) { ?>
        Column col_<?=$att?>;
        <?=$att?>_Column_Out.Done(col_<?=$att?>);
        output.SwapColumn(col_<?=$att?>, <?=$att->slot()?>);
<?      } // foreach output att ?>
    } // Matches whichOne
<?  } // foreach query ?>

    ChunkContainer tempResult(output);
    tempResult.swap(result);

    return WP_FINALIZE; // ProduceResults
}

<?
}

function GISTGenerate_ProduceState($wpName, $queries, $attMap) {
?>

extern "C"
int GISTProduceStateWorkFunc_<?=$wpName?>
(WorkDescription &workDescription, ExecEngineData &result) {
    GISTProduceResultsWD myWork;
    myWork.swap(workDescription);

    // Inputs
    QueryExit& whichOne = myWork.get_whichOne();
    GLAState& gist = myWork.get_gist();

<?  cgDeclareQueryIDs($queries); ?>

<?  foreach ($queries as $query => $info) {
        $gist = $info['gist'];
        if ($gist->finalize_as_state()) { ?>
    // do <?=queryName($query)?>
    if (whichOne.query == <?=queryName($query)?>){
        // look for the state of <?=queryName($query)?>
        GLAPtr state;
        state.swap(gist);

        FATALIF(state.get_glaType() != <?=$gist->cHash()?>,
            "Got GIST of unexpected type when finalizing as state for query <?=queryName($query)?>");

        <?=$gist?>* state_<?=queryName($query)?> = (<?=$gist?>*) state.get_glaPtr();
        state_<?=queryName($query)?>->FinalizeState();
        state.swap(gist);
    }
<?      }
    } ?>

    WayPointID myID = WayPointID::GetIdByName("<?=$wpName?>");
    StateContainer stateCont( myID, whichOne, gist );
    stateCont.swap(result);

    return WP_FINALIZE; // Produce Results
}

<?
}
?>
