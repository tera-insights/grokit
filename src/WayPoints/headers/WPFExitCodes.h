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
#ifndef WPF_EXIT_CODES_H
#define WPF_EXIT_CODES_H

enum WPFExitCode {
    // Initial processing needed for a query. Usually done only once per query
    // for the lifetime of a waypoint.
    WP_PREPROCESSING   = -2,
    // Any processing needed before each round of data.
    WP_PREPARE_ROUND   = -1,
    // Processing of chunks of tuples.
    WP_PROCESS_CHUNK   =  0,
    // Any non-chunk based processing.
    WP_PROCESSING      =  1,
    // Usually done after all data has been processed.
    WP_POST_PROCESSING =  2,
    // Prepares the query for output.
    WP_PRE_FINALIZE    =  3,
    // Generates output.
    WP_FINALIZE        =  4,
    // Cleans up after a query.
    WP_POST_FINALIZE   =  5
};

#endif
