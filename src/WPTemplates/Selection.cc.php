<?

/* Copyright 2013, Tera Insights. All rights Reserved */

/* function to instantiate a Selection waypoint */

function SelectionGenerate($wpName, $queries, $attMap) {

    //echo PHP_EOL . '/*' . PHP_EOL;
    //print_r($wpName);
    //print_r($queries);
    //print_r($attMap);
    //echo PHP_EOL . '*/' . PHP_EOL;
?>

// module specific headers to allow separate compilation
#include "GLAData.h"
#include "Errors.h"

//+{"kind":"WPF", "name":"Pre-Processing", "action":"start"}
extern "C"
int SelectionPreProcessWorkFunc_<?=$wpName?>

(WorkDescription& workDescription, ExecEngineData& result) {
    SelectionPreProcessWD myWork;
    myWork.swap(workDescription);
    QueryExitContainer& queries = myWork.get_whichQueryExits();
    QueryToGLASContMap & requiredStates = myWork.get_requiredStates();

    QueryToGLAStateMap constStates;
<?
    cgDeclareQueryIDs($queries);
?>

<?  foreach( $queries as $query => $info ) {
        $gf = $info['gf'];
        if( !is_null($gf) && $gf->has_state() ) {
            $state = $gf->state();
            if( $state->configurable() ) {
                $carg = $info['cargs'];
                echo '    // JSON Configuration for query ' . queryName($query) . PHP_EOL;
                $carg->init();
                echo PHP_EOL;
            } // if gf const state is configurable
        } // if gf has state
    } //foreach query
?>

    FOREACH_TWL(iter, queries) {
<?
    foreach($queries  as $query=>$val) {
?>
        if( iter.query == <?=queryName($query)?>) {
<?
        if( $val['gf'] !== null ) {
            // This is a generalized filter
            $gf = $val['gf'];
            $given_states = $val['states'];

            if( $gf->has_state() ) {
                $cstArgs = [];
                $state = $gf->state();

                // If the state is configurable, give it the JSON carg
                if( $state->configurable() ) {
                    $carg = $query['cargs'];
                    $cstArgs[] = $carg->name();
                } // if gf state is configurable

                if( \count($given_states) > 0 ) {
?>
            FATALIF(!requiredStates.IsThere(<?=queryName($query)?>),
                "No required states received for query that declared required states");
            GLAStateContainer& givenStates = requiredStates.Find(<?=queryName($query)?>);
            givenStates.MoveToStart();
            GLAPtr reqTemp;
<?
                    foreach( $givenStates as $gs ) {
                        $cstArgs[] = $gs->name();
?>
            // Extract state from waypoint[<?=$gs->waypoint()?>]
            <?=$gs->type()?> * <?=$gs->name()?> = nullptr;
            reqTemp.Swap(givenStates.Current());
            FATALIF( reqTemp.get_glaType() != <?=$gs->type()->cHash()?>,
                "Got different type than expected for required state of type <?=$gs>type()?>");
            <?=$gs->name()?> = (<?=$gs->type()?> *) reqTemp.get_glaPtr();
            reqTemp.swap(givenStates.Current());
            givenStates.Advance();
<?
                    } // foreach given state
                } // if we have given states

                $cstStr = \count($cstArgs) > 0 ? '(' . implode(', ', $cstArgs) . ')' : '';
?>
            <?=$state?> * temp = new <?=$state?><?=$cstStr?>;
            GLAPtr newPtr( <?=$state->cHash()?>, (void *) temp );
            QueryID qryID = <?=queryName($query)?>;
            constStates.Insert(qryID, newPtr);
<?
            } // if gf has state
        } // if( $val['gf'] !== null )
?>
        } // if <?=queryName($query)?> is current query
<?
    } // foreach query
?>
  } END_FOREACH;

  SelectionPreProcessRez myRez( constStates );
  myRez.swap(result);

  return WP_PREPROCESSING; // for PreProcess
}
//+{"kind":"WPF", "name":"Pre-Processing", "action":"end"}

//+{"kind":"WPF", "name":"Process Chunk", "action":"start"}
extern "C"
int SelectionProcessChunkWorkFunc_<?=$wpName?> (WorkDescription &workDescription, ExecEngineData &result) {
    // go to the work description and get the input chunk
    SelectionProcessChunkWD myWork;
    myWork.swap (workDescription);
    Chunk &input = myWork.get_chunkToProcess ();
    QueryToGLAStateMap& constStates = myWork.get_constStates();

    PROFILING2_START;

    QueryIDSet queriesToRun = QueryExitsToQueries(myWork.get_whichQueryExits ());
<?
    cgDeclareQueryIDs($queries);
    cgAccessColumns($attMap, 'input', $wpName);

    // Declare the constants needed by the filters and synth expressions.
    foreach( $queries as $query => $val ) {
?>
    // Constants for query <?=queryName($query)?>:
<?
        $filters = $val['filters'];
        $synths = $val['synths'];

        cgDeclareConstants($filters);
        cgDeclareConstants($synths);
    } // foreach query
?>

    // prepare bitstring iterator
    Column inBitCol;
    BStringIterator queries;
    input.SwapBitmap (queries);

    // creating storage for syhthesized attributes
<?
    foreach($queries as $query => $val) {
        $synList = $val['synths'];
        foreach($synList as $att => $syn) {
?>
    MMappedStorage <?=attStorage($att)?>;
    Column <?=attCol($att)?>(<?=attStorage($att)?>);
    <?=attIteratorType($att)?> colI_<?=$att?>(<?=attCol($att)?>);
    <?=attType($att)?> <?=$att?>;
<?
        } // foreach synthesized attribute
    } // foreach query
?>


<?
    foreach($queries as $query => $val) {
        $givenStates = $val['states'];
        $gf = $val['gf'];
        $cargs = $val['cargs'];

        grokit_assert( $gf !== null || count($givenStates) == 0,
            'We were given states for query '. $query . ' when we have no GF!');

        if( !is_null($gf) && $gf->has_state() ) {
            $state = $gf->state();
            $stateName = 'cst_state_' . queryName($query);
            $constMod = $state->mutable() ? '' : 'const ';
?>
    // Extracting constant state for query <?=queryName($query)?>
    FATALIF(!constStates.IsThere(<?=queryName($query)?>), "No constant state found for query <?=queryName($query)?>.");
    <?=$constMod?><?=$state?> * <?=$stateName?> = nullptr;
    {
        GLAState& curState = constStates.Find(<?=queryName($query)?>);
        GLAPtr tmp;
        tmp.swap(curState);
        FATALIF( tmp.get_glaType() != <?=$state->cHash()?>,
            "Got different type than expected for constant state of type <?=$state?>");
        <?=$stateName?> = (<?=$constMod?><?=$state?> *) tmp.get_glaPtr();
        tmp.swap(curState);
    }

<?
        } // if gf requires constant state

        if( $gf !== null ) {
            $ctrArgs = [];

            if( $gf->configurable() ) {
                echo '    // JSON initialiser for query '. queryName($query) . PHP_EOL;
                $cargs->init();
                echo PHP_EOL;
                $ctrArgs[] =$cargs->name();
            }

            if( $gf->has_state() ) {
                $ctrArgs[] = '*'. $stateName;
            }

            $ctrStr = \count($ctrArgs) > 0 ? '(' . implode(', ', $ctrArgs) . ')' : '';
?>
    // Construct GF for query <?=queryName($query)?>

    <?=$gf->value()?> <?=queryName($query)?>_state<?=$ctrStr?>;
<?
        } // if we have a GF
    } // foreach query
?>

    MMappedStorage bitStore;
    Column outBitCol(bitStore);
    BStringIterator outQueries (outBitCol, queriesToRun);

#ifdef PER_QUERY_PROFILE
<?
    foreach($queries as $query => $val){
?>
    int64_t n_tuples_<?=queryName($query)?> = 0;
<?
    } // foreach query
?>
#endif // PER_QUERY_PROFILE

    int64_t numTuples = 0;
    while (!queries.AtEndOfColumn ()) {
        ++numTuples;
        QueryIDSet qry;
        qry = queries.GetCurrent();
        qry.Intersect(queriesToRun);
        queries.Advance();

        //selection code for all the predicates
<?
    cgAccessAttributes($attMap);
    foreach($queries as $query => $val) {
        $gf = $val['gf'];
        $filters = $val['filters'];
        $synths = $val['synths'];
        $stateName = queryName($query) . '_state';

        $filterVals = array_map( function($expr) { return '('. $expr . ')'; }, $filters );
        if( $gf === null ) {
            // Simple set of expressions.
            if( \count($filterVals) > 0 )
                $selExpr = implode( ' && ', $filterVals );
            else
                $selExpr = 'true';
        }
        else {
            // We have a GF
            $selExpr = "{$stateName}.Filter(" . implode( ', ', $filterVals) . ")";
        }
?>
        // do <?=queryName($query)?>:
<?
        foreach($synths as $att => $syn) {
?>
        <?=attType($att)?> <?=$att?>;
<?
        } // foreach synthesized attribute
?>
        if( qry.Overlaps(<?=queryName($query)?>) ) {
#ifdef PER_QUERY_PROFILE
            ++numTuples_<?=queryName($query)?>;
#endif // PER_QUERY_PROFILE
<?          cgDeclarePreprocessing($filters, 2); ?>
            if( <?=$selExpr?> ) {
                // compute synthesized
<?
        cgDeclarePreprocessing($synths, 3);
        foreach($synths as $att => $expr ) {
?>
            <?=$att?> = <?=$expr?>;
<?
        } //foreach synthesized attribute
?>
            } else {
                qry.Difference(<?=queryName($query)?>);
            }
        }
<?
        foreach($synths as $att => $syn) {
?>
            colI_<?=$att?>.Insert(<?=$att?>);
            colI_<?=$att?>.Advance();
<?
        } // foreach synthesized attribute
    } // foreach query
?>
        outQueries.Insert(qry);
        outQueries.Advance();

<?
    cgAdvanceAttributes($attMap);
?>
    } // while we still have tuples remaining

    // finally, if there were any results, put the data back in the chunk
<?
    cgPutbackColumns($attMap, 'input', $wpName);
    foreach( $queries as $query => $val ) {
        $synths = $val['synths'];
?>
    if (<?=queryName($query)?>.Overlaps(queriesToRun)) {
<?
        foreach( $synths as $att => $expr ) {
?>
        colI_<?=$att?>.Done(<?=attCol($att)?>);
        input.SwapColumn(<?=attCol($att)?>, <?=attSlot($att)?>);
<?
        } //foreach synthesized attribute
?>
    } // If <?=queryName($query)?> overlaps queriesToRun
<?
    } // foreach query
?>

    // put in the output bitmap
    outQueries.Done ();
    input.SwapBitmap (outQueries);

    // Finish performance counters
    PROFILING2_END;

    PCounterList counterList;
    PCounter totalCnt("tpi", numTuples, "<?=$wpName?>");
    counterList.Append(totalCnt);
    PCounter tplOutCnt("tpo", numTuples, "<?=$wpName?>");
    counterList.Append(tplOutCnt);

#ifdef PER_QUERY_PROFILE
<?
    foreach($queries as $query => $val) {
        $filters = $val['filters'];
?>
    if( <?=queryName($query)?>.Overlaps(queriesToRun)) {
        PCounter cnt("<?=queryName($query)?>", numTuples_<?=queryName($query)?>, "<?=$wpName?>");
        counterList.Append(cnt);
    }
<?
    } // foreach query
?>
#endif // PER_QUERY_PROFILE

    PROFILING2_SET(counterList, "<?=$wpName?>");

    ChunkContainer tempResult (input);
    tempResult.swap (result);

    return WP_PROCESS_CHUNK; // For Process Chunk
}
//+{"kind":"WPF", "name":"Process Chunk", "action":"end"}

<?
    } // SelectionGenerate function
?>
