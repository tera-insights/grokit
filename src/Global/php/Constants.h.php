<?php
//
//  Copyright 2012 Alin Dobra and Christopher Jermaine
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
require_once('grokit_config.php');
?>
//
//  Copyright 2012 Alin Dobra and Christopher Jermaine
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//

#ifndef _CONSTANTS_H_
#define _CONSTANTS_H_



/*
==================Central hash table parameters==================
* - NUM_SLOTS_IN_SEGMENT_BITS: This should not be over 24 bits if the size of a chunk is 2M tuples.
*     This size works well and Cleaner produces reasonably sized chunks.
* - NUM_SEGS: This should be manipulated to use most memory in the system (about 70%).
*     Make sure it is set to a sum of few 2^k numbers so that % operator is implemented
*     efficiently by the compiler.
*/
#define NUM_SLOTS_IN_SEGMENT_BITS <?=$__grokit_config_segment_bits?>

#define NUM_SEGS <?=$__grokit_config_num_segs?>



/*
==================Column related constants==================
* - COLUMN_ITERATOR_STEP: Constant to determine how much data is prepared in advance in column iterators
* - COMPRESSION_UNIT: To allowed streamed decompression, small units have to be compressed in a streaming fashion.
*/
#define COLUMN_ITERATOR_STEP (1<<14) /* 16KB */
#define COMPRESSION_UNIT COLUMN_ITERATOR_STEP



/*
==================Logger Constants==================
*/
#define LOG_FILE "LOG"
#define LOG_LEVEL 10



/*
==================Metadata filename==================
*/
#define METADATA_FILE "datapath.sqlite"


/*
================== System Information =================
*/

// Check if we have C++11 support

#if __cplusplus >= 201103L
    #define _HAS_CPP_11
#endif

/*
==================Other Settings==================
*/

/* Execution engine heart bit. Expressed in seconds
*/
#define HEARTBEAT_EE 5


/* Number of tuples in a chunk produced.
*/
#define PREFERED_TUPLES_PER_CHUNK ( 2*1024*1023 )


/* Number of threads available for the execution engine. This should be # Processors x 1.5
*/
#define NUM_EXEC_ENGINE_THREADS <?=$__grokit_config_exec_threads?>


/* How many disk tokens we allow (this controls the parallelism)
*/
#define NUM_DISK_TOKENS <?=$__grokit_config_disk_tokens?>


/* Maximum number of chunks that can be built in parallel by the file scanner.
*/
#define FILE_SCANNER_MAX_NO_CHUNKS_REQUEST 5


/* Fraction of threads that need to be available to use compressed data
*/
#define USE_UNCOMPRESSED_THRESHOLD .1


/* Maximum number of threads running in the ChunkReaderWriter (serving messages).
*/
#define CHUNK_RW_THREADS 12


/* Duration between disk operation statistics computation.
   The smaller the value, the more often the statistics are computed.
*/
#define DISK_OPERATION_STATISTICS_INTERVAL 100


/* This is the number of CPU work token requests that the hash table cleaner can have out at one time.
*/
#define MAX_CLEANER_CPU_WORKERS <?=$__grokit_config_cpu_cleaners?>


/* This is the number of disk work tokens that the cleaner can hoard to give to writers
*/
#define MAX_CLEANER_DISK_REQUESTS <?=$__grokit_config_cleaner_disk_requests?>

#endif //_CONSTANTS_H_
