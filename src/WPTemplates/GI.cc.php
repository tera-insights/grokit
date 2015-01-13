<?php

/* Copyright 2013, Tera Insights. All rights Reserved */

// Function to instantiate a GI Waypoint.
function GIGenerate( $wpName, $type, $outputs, $constArgs, $tuples ) {
    // put an element in front to form constant arguments
    array_unshift($constArgs, "stream_info");
    $constArgsStr = join($constArgs, ',');

    // comma separated list of outputs
    $outputArgs = join( array_map( function($el){ return $el->name(); }, $outputs), ',');
?>


#include "Dictionary.h"
#include "DictionaryManager.h"
#include "GIStreamInfo.h"
#include "Profiling.h"
#include <iostream>

//+{"kind":"WPF", "name":"Produce Chunk", "action":"start"}
extern "C"
int GIProduceChunkWorkFunc_<?=$wpName?> ( WorkDescription &workDescription, ExecEngineData &result) {
    const size_t maxTuplesPerChunk = <?=$tuples == 0 ? "PREFERED_TUPLES_PER_CHUNK" : $tuples ?>;

    GIProduceChunkWD myWork;
    myWork.swap( workDescription );

    GLAState &gi_state = myWork.get_gi();
    GIStreamProxy &stream_info = myWork.get_stream_info();

    QueryIDSet queriesToRun = myWork.get_queriesCovered();

    GLAPtr state_ptr;
    state_ptr.swap(gi_state);

    <?=$type->value()?> * my_state = NULL;

    if( state_ptr.IsValid() ) {
        my_state = (<?=$type->value()?> *) state_ptr.get_glaPtr();
    } else {
        my_state = new <?=$type->value()?> ( <?=$constArgsStr?> );
        GLAPtr newPtr(<?=$type->cHash()?>, (void*) my_state);
        newPtr.swap(state_ptr);
    }

    state_ptr.swap(gi_state);

    // Output chunk
    Chunk chunk;

    PROFILING2_START;
    // Start new columns and allocate storage for them
<?  cgConstructColumns($outputs); ?>

    size_t tuple_count = 0;
    bool stream_done = false;

    while( !stream_done && tuple_count < maxTuplesPerChunk ) {
        if( __builtin_expect( my_state->ProduceTuple( <?=$outputArgs?> ), 1) ) {
            ++tuple_count;
<?  foreach($outputs as $att){ ?>
            <?=$att->name()?>_Column_Out.Insert(<?=$att->name()?>);
            <?=$att->name()?>_Column_Out.Advance();
<?  }?>
        }
        else {
            stream_done = true;
        }
    }

    // Deal with PreDone() calls to release read locks
<?  foreach($outputs as $att){
    if ($att->type()->reqDictionary()) { ?>
    <?=$att->name()?>_Column_Out.PreDone();
    <?  }
    } ?>

    // Finalize output columns and put them into the chunk.
<?  foreach($outputs as $att){
    if ($att->type()->reqDictionary()) {
?>
    <?=$att->name()?>_Column_Out.Done(<?=$att->name()?>_Column_Ocol,
    my_state->get_dictionary_<?=$att->type()->dictionary()?>());
    <?  } else { ?>
    <?=$att->name()?>_Column_Out.Done(<?=$att->name()?>_Column_Ocol);
    <?  } ?>
    {
        int cSlot = <?=$att->slot()?>;

        FATALIF( ! <?=$att->name()?>_Column_Ocol.IsValid(), "Column _A_ invalid when packing into chunk!");
        chunk.SwapColumn( <?=$att->name()?>_Column_Ocol, cSlot );
    }
<?  } ?>


    PROFILING2_END;
    PROFILING2_SINGLE("tpo", tuple_count, "<?=$wpName?>");

    MMappedStorage bitStore;
    Column outBitCol(bitStore);
    BStringIterator outQueries( outBitCol, queriesToRun, tuple_count );
    outQueries.Done();
    chunk.SwapBitmap(outQueries);

    ChunkContainer chunkCont(chunk);

    // pack the chunk, stream, and GI into the result and send it back
    GIProduceChunkRez tempResult( stream_info, gi_state, chunkCont );
    result.swap(tempResult);

    if( stream_done )
    {
        // Deallocate GI
        delete my_state;
        GLAState blankState;
        blankState.swap(gi_state);
        return 1;
    }
    else{
        return 0;
    }
}
//+{"kind":"WPF", "name":"Produce Chunk", "action":"end"}

<?
}
?>
