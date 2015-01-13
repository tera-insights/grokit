<?
function CacheGenerate( $wpName ) {
?>

#include "WPFExitCodes.h"

extern "C"
int CacheChunkWorkFunc_<?=$wpName?>
(WorkDescription& workDescription, ExecEngineData& result) {
    CacheChunkWD myWork;
    myWork.swap(workDescription);

    ChunkContainer cont;
    cont.swap(myWork.get_chunkToProcess());

    cont.swap(result);

    return WP_PROCESS_CHUNK;
}
<?
} // function CacheGenerate
?>
