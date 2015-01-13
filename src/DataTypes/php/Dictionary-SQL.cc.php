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
require_once('SQLite.php');
?>



/** Implementation of the FactorsManager methods. Need access to the
dictionary to do this. */

#include "Dictionary.h"
#include "DictionaryManager.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "Constants.h"
#include "MetadataDB.h"

#include <sstream>
#include <iostream>

void Dictionary::Load(const char* name){
<?php
grokit\sql_open_database( 'GetMetadataDB() ' );
?>
;

    // create relation that holds the data if it doesn't exist.
<?php
grokit\sql_statements_norez( <<<'EOT'
"
    CREATE TABLE IF NOT EXISTS "Dictionary_%s" (
        "id"          INTEGER,
        "order"       INTEGER,
        "str"           TEXT);
"
EOT
, [ 'name', ] );
?>
;

    // Clear existing data from maps
    indexMap.clear();
    reverseMap.clear();

<?php
grokit\sql_statement_table( <<<'EOT'
"
    SELECT "id", "order", "str" FROM "Dictionary_%s";
"
EOT
, [ 'id' => 'int', 'order' => 'int', 'str' => 'text', ], [ 'name', ] );
?>
    {
        StringType s(str);
        indexMap[id] = str;
        reverseMap[str] = id;
        orderMap[id] = order;

        if (nextID <= id)
            nextID = id+1;

    }
<?php
grokit\sql_end_statement_table();
?>
;

<?php
grokit\sql_close_database();
?>
;

    modified = false;
    orderValid = true;
}

void Dictionary::Save(const char* name){
    if (!modified)
        return;

<?php
grokit\sql_open_database( 'GetMetadataDB() ' );
?>
;

<?php
grokit\sql_statements_norez( <<<'EOT'
"
DELETE FROM "Dictionary_%s";
"
EOT
, [ 'name', ] );
?>
;

<?php
grokit\sql_statement_parametric_norez( <<<'EOT'
"
    INSERT INTO "Dictionary_%s" ("id", "order", "str") VALUES (?1, ?2, ?3);
"
EOT
, [ 'int', 'int', 'text', ], [ 'name', ]);
?>
;
    // iterate through the dictionary
    for( IndexMap::const_iterator it = indexMap.begin(); it != indexMap.end(); ++it ) {
        IntType id = it->first;
        StringType s = it->second;
        IntType order = orderMap[id];
        const char * str = s.c_str();
<?php
grokit\sql_instantiate_parameters( [ 'id', 'order', 'str', ] );
?>
;
    }
<?php
grokit\sql_parametric_end();
?>
;

<?php
grokit\sql_close_database();
?>
;

  modified = false;
}
