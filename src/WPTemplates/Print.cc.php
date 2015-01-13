<?

/* Copyright 2013, Tera Insights. All rights Reserved */

/* function to instantiate a Print waypoint */

function PrintGenerate($wpName, $queries, $attMap){
?>

// module specifsic headers to allow separate compilation
#include <iostream>
#include <string.h>
#include "Profiling.h"

//+{"kind":"WPF", "name":"Process Chunk", "action":"start"}
extern "C"
int PrintWorkFunc_<?=$wpName?> (WorkDescription &workDescription, ExecEngineData &result) {

    // get the work description
    PrintWorkDescription myWork;
    myWork.swap (workDescription);
    Chunk &input = myWork.get_chunkToPrint ();
    QueryToFileMap& streams = myWork.get_streams();
    QueryToCounters& counters = myWork.get_counters();

    QueryIDSet queriesToRun = QueryExitsToQueries(myWork.get_whichQueryExits ());
    // prepare bitstring iterator
    Column inBitCol;
    BStringIterator queries;
    input.SwapBitmap (queries);

<?
    cgDeclareQueryIDs($queries);
    cgAccessColumns($attMap, 'input', $wpName);
    cgConstantInit($queries);

?>

    // for each query, define a stream variable
<?
    foreach($queries as $query => $val) {
        $type = $val["type"];

        if( $type == 'json' ) {
            // Need some extra variables
?>
    Json::Value json;
    Json::Value jsonRow;
    Json::FastWriter jsonWriter;
    std::string jsonString;
<?      } // if type is json ?>

    PrintFileObj& pfo_<?=queryName($query)?> = streams.Find(<?=queryName($query)?>);
    DistributedCounter* counter_<?=queryName($query)?> = counters.Find(<?=queryName($query)?>);
    FILE* file_<?=queryName($query)?> = pfo_<?=queryName($query)?>.get_file();
    const char * DELIM_<?=queryName($query)?> = "<?=$type == 'json' ? ',' : $val["separator"]?>";
#ifdef PER_QUERY_PROFILE
    size_t n_tuples_<?=queryName($query)?> = 0;
#endif // PER_QUERY_PROFILE
<?
    } // foreach query
?>
    // PRINTING
    constexpr const size_t BUFFER_LENGTH = 10 * 1024 * 1024; // 10 MB
    char buffer[BUFFER_LENGTH]; // ALIN, CHANGE THIS TO A DEFINED CONSTANT

    PROFILING2_START;
    size_t n_tuples = 0;
    while (!queries.AtEndOfColumn ()){
        ++n_tuples;
        QueryIDSet qry;
        qry = queries.GetCurrent();
        qry.Intersect(queriesToRun);
        queries.Advance();

<?  cgAccessAttributes($attMap);
    foreach($queries as $query=>$val){ ?>
        // execute <?=queryName($query)?> code
        if (qry.Overlaps(<?=queryName($query)?>) && counter_<?=queryName($query)?>->Decrement(1)>=0){
<?      cgPreprocess($val); ?>
#ifdef PER_QUERY_PROFILE
            ++n_tuples_<?=queryName($query)?>;
#endif // PER_QUERY_PROFILE
            int curr=0; // the position where we write the next attribute

<?      if( $type == 'json' ) { ?>
            jsonRow = Json::Value(Json::arrayValue);

<?          foreach( $val["expressions"] as $exp) { ?>
            ToJson(<?=$exp->value()?>, json);
            jsonRow.append(json);
<?          } // for each expression ?>
            
            jsonString = jsonWriter.write(jsonRow);
            fprintf(file_<?=queryName($query)?>, "%s,", jsonString.c_str());
<?      } // if output file is json
        else if( $type == 'csv' ) {
            foreach($val["expressions"] as $exp) {
?>
            curr += ToString(<?=$exp->value()?>,buffer+curr);
            curr += sprintf(buffer + (curr-1), "%s", DELIM_<?=queryName($query)?>) - 1;
<?          } // for each expression ?>

            // Replace the last comma with a newline
            buffer[curr-1]='\n';

            // Null terminate the string
            buffer[curr]='\0';

            // Now we print the buffer
            fprintf(file_<?=queryName($query)?>, "%s", buffer);
<?      } // if output file is csv ?>
        }
<?  } // for each query ?>
<?
    cgAdvanceAttributes($attMap);
?>
    }

<?  cgPutbackColumns($attMap, 'input', $wpName); ?>

    PROFILING2_END;

    PCounterList counterList;
    PCounter totalCnt("tpi", n_tuples, "<?=$wpName?>");
    counterList.Append(totalCnt);

#ifdef PER_QUERY_PROFILE
    // add query counters to list
<?foreach($queries as $query=>$val){ ?>
    {
        PCounter cnt("<?=queryName($query)?>", n_tuples_<?=queryName($query)?>, "<?=$wpName?>");
        counterList.Append(cnt);
    }
<?}?>
#endif // PER_QUERY_PROFILE

    PROFILING2_SET(counterList, "<?=$wpName?>");

    // just return some arbitrary value... don't worry about reconstructing the chunk
    return WP_PROCESS_CHUNK;
}
//+{"kind":"WPF", "name":"Process Chunk", "action":"end"}

//+{"kind":"WPF", "name":"Finalize", "action":"start"}
extern "C"
int PrintFinalizeWorkFunc_<?=$wpName?> (WorkDescription &workDescription, ExecEngineData &result) {

    PrintFinalizeWorkDescription myWork;
    myWork.swap( workDescription );
    QueryToFileMap& streams = myWork.get_streams();

    QueryIDSet queriesToRun = QueryExitsToQueries(myWork.get_whichQueryExits());

<?  cgDeclareQueryIDs($queries); ?>

    // For each query, define a stream variable
<?
    $jsonVarsDefined = false;
    foreach( $queries as $query => $val ) {
        $type = $val['type'];

        if( $type == 'json' && !$jsonVarsDefined ) {
            $jsonVarsDefined = true;
?>
    Json::Value json;
    Json::FastWriter jsonWriter;
    std::string jsonString;
<?
        } // if we need to define extra json vars

?>
    PrintFileObj& pfo_<?=queryName($query)?> = streams.Find(<?=queryName($query)?>);
    FILE* file_<?=queryName($query)?> = pfo_<?=queryName($query)?>.get_file();
<?
    } // for each query
?>

<?  foreach( $queries as $query => $val ) {
        $type = $val['type'];
        if( $type == 'json' ) {
?>
    // Set up the types array
    json = Json::Value(Json::arrayValue);

<?          foreach( $val['expressions'] as $exp ) {
                $describer = $exp->type()->describer('json');
                grokit_assert(is_callable($describer), 'Invalid JSON describer for type ' . $exp->type());
?>
    {
        Json::Value tmp;
        <? $describer('tmp'); ?>
        json.append(tmp);
    }
<?          } // for each expression ?>

    jsonString = jsonWriter.write(json);
    // Last character is a newline, remove it
    jsonString.erase(jsonString.size() - 1, 1);

    // End the content section and write out the types
    fseek(file_<?=queryName($query)?>, -1, SEEK_CUR); // overwrite the last comma
    fprintf(file_<?=queryName($query)?>, " ], \"types\": %s }", jsonString.c_str());

<?      } // if type is json ?>
<?  } // for each query ?>

    return WP_FINALIZE;
}
//+{"kind":"WPF", "name":"Finalize", "action":"end"}
<?
} // PrintGenerate function
?>
