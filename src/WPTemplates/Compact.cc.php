<?
function CompactGenerate($wpName, $attMap) {
?>

#include "WPFExitCodes.h"

// This is the function that does all the work for the Compact waypoint. First,
// the chunk is checked whether all of the tuples are turned on for each exiting
// query, i.e. the chunk is dense for each exiting query. If so, the chunk is
// simply passed through. Otherwise, the tuples are iterated over and only the
// ones turned on for at least one exiting query are kept. Note that this allows
// for the case where every tuple is kept.

// For example, consider a chunk with two tuples and two queries. If the tuples'
// bitstrings are 01 and 10, then the chunk is not dense for either query, but
// all tuples will be passed through.

// This strategy means that a Compact that is followed by a Store should only
// have a single exiting query, which guarantees that all tuples kept are valid
// for the Store. This is important because a Store keeps all tuples in a chunk,
// regardless of whether they are turned off.
extern "C"
int CompactProcessChunkWorkFunc_<?=$wpName?>(WorkDescription& workDescription,
                                             ExecEngineData& result) {
  // The work description is specialized.
  CacheChunkWD myWork;
  myWork.swap(workDescription);

  // The chunk to be processed is taken.
  Chunk& chunk = myWork.get_chunkToProcess();

  // The performance profiling is started.
  PROFILING2_START;
  int64_t num_tuples = 0;

  // The exiting queries are processed and placed in a bit string.
  QueryIDSet queries_to_run = QueryExitsToQueries(myWork.get_whichQueryExits());

  // This is the bit string iterator for the chunk being processed.
  BStringIterator input_iterator;
  chunk.SwapBitmap(input_iterator);

  // The chunk only needs to be dense for each exiting query, i.e. the set of
  // exiting queries is a subset of the queries for which the chunk is dense.
  bool is_dense = !(queries_to_run & ~input_iterator.GetDenseQueries());

  if (is_dense) {
    // No changes are needed for this chunk and it simply returned.
    ChunkContainer temp_result(chunk);
    temp_result.swap(result);
  } else {
    // The chunk is not dense, meaning some tuples are turned off. These tuples
    // are removed and new columns are created.

    // This is the column for the bit strings for the output.
    MMappedStroage bit_string_storage;
    Column bit_string_col(bit_string_storage);
    BStringIterator output_iterator(bit_string_col, queries_to_run);

    // These are the columns and their iterators for the input and output.
<?  cgAccessColumns($attMap, 'chunk', $wpName); ?>
<?  cgConstructColumns(array_keys($attMap), "_Copy"); ?>

    // The input chunk is iterated over.
    while (!input_iterator.AtEndOfColumn()) {
      num_tuples++;
      // The bits for non-exiting queries are stripped from the current string.
      QuerryIDSet bits = input_iterator.GetCurrent().Intersect(queries_to_run);
      // The tuple is kept if it is turned on for any of the exiting queries.
      if (bits) {
        // The column information is copied over.
<?  cgAccessAttributes($attMap); ?>
<?  foreach ($attMap as $att => $expr) { ?>
        <?=$att?>_Copy_Column_Out.Insert(<?=$att?>);
<?  } ?>
        output_iterator.Insert(bits);

        // The output columns are advanced because an insertion occured.
<?  foreach ($attMap as $att => $expr) { ?>
        <?=$att?>_Copy_Column_Out.Advance();
<?  } ?>
        output_iterator.Advance();
      }
      // All of the input columns are advanced, including the bit strings.
<?  cgAdvanceAttributes($attMap); ?>
      input_iterator.Advance();
    }

    // The various information is inserted into the chunk.
<?  foreach ($attMap as $att => $expr) { ?>
    <?=$att?>_Copy_Column_Out.Done();
    chunk.SwapColumn(<?=attCol($att)?>, <?=attSlot($att)?>);
<?  } ?>
    chunk.SwapBitmap(output_iterator);

    // The chunk is outputted.
    ChunkContainer temp_result(chunk);
    temp_result.swap(result);
  }

  // The performance profiling is finished.
  PROFILING2_END;
  PCounterList counterList;
  PCounter totalCnt("tpi", num_tuples, "<?=$wpName?>");
  counterList.Append(totalCnt);
  PCounter tplOutCnt("tpo", num_tuples, "<?=$wpName?>");
  counterList.Append(tplOutCnt);
  PROFILING2_SET(counterList, "<?=$wpName?>");

  return WP_PROCESS_CHUNK;
}
//+{"kind":"WPF", "name":"Process Chunk", "action":"end"}

<?
}
?>
