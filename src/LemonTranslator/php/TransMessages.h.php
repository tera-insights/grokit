/**
    This header file contains the messages used by DiskIO.
*/

<?php
//
//  Copyright 2013 Tera Insights LLC
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
?>
<?php
require_once('MessagesFunctions.php');
?>



#ifndef _TRANS_MESSAGES_H_
#define _TRANS_MESSAGES_H_

#include "Message.h"
#include "DataPathGraph.h"
#include "WayPointConfigureData.h"

#include "Tasks.h"

#include <string>

////////// SYMBOLIC QUERY DESCRIPTIONS /////////

/** Message sent by the Translator to the Coordinator to give it the symbolic
        description. This description contains both information for
        the CodeGenerator and for the ExecutionEngine.
        The info for the FileScanners is sent directly.

        Arguments:
            newGraph: the new graph for the execution engine
            wpDesc: symbolic configuration for the WayPoints
*/
<?php
grokit\create_message_type( 'SymbolicQueryDescriptions', [ ],
    [   'newQueries' => 'QueryExitContainer',
    'newGraph' => 'DataPathGraph',
    'wpDesc' => 'WayPointConfigurationList',
    'tasks' => 'TaskList',
    ]
);
?>



///// LEMON TRANSLATOR MESSAGES //////

/** Message is only used to implement the SimpleTranslator.
        It is going to be changed for the full translator.
        The message is sent by the Coordinator to the SimpleTranslator.

        Arguments:
            file: full path to the file with instructions for translator. dp file
*/
<?php
grokit\create_message_type( 'TranslationMessage', [ 'confFile' => 'std::string', ], [ ], true );
?>


/** Message to specify that a set of queries have finished

        Arguments:
                queries: the set of queries that finished
*/
<?php
grokit\create_message_type( 'DeleteQueriesMessage', [ 'queries' => 'QueryIDSet', ], [ ], true );
?>



#endif // _TRANS_MESSAGES_H_
