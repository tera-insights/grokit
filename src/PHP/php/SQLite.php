<?php
namespace grokit;

// Copyright 2013 Christopher Dudley
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/*
 * This file contains helper functions for managing SQLite connections and
 * queries in C/C++.
 *
 * NOTE: All queries must be given to these functions already quoted!
 * This is the only way I could think to allow both literal queries and
 * queries passed via a const char * without doing weird things.
 *
 */

require_once('grokit_base.php');

function multiline_string( $str ) {
    $str = trim($str);
    // If the string isn't quoted, it's a variable, so just return it.
    if( $str[0] != '"' ) {
        return $str;
    }

    $lines = explode("\n", $str);
    $res = '';

    foreach( $lines as $line ) {
        $clean_line = trim($line, '"');
        $clean_line = rtrim($clean_line);
        $clean_line = addcslashes($clean_line, '"\\');

        if( $clean_line != '' )
            $res .= "\"$clean_line \"\n";
    }

    return rtrim($res);
}

// Executes one or more statements without a result.
// $query   : command in "" or a char * with the command.
// $args    : optional argument for printf like behavior.
function sql_statements_norez( $query, array $args = [] ) {
?>

    zSql_INTERNAL = <?=multiline_string($query)?>;

<?php if( count($args) > 0 ) { ?>
    sprintf(buffer_INTERNAL, zSql_INTERNAL, <?=implode(',', $args)?>);
    zSql_INTERNAL = buffer_INTERNAL;
<?php } ?>

    PDEBUG(zSql_INTERNAL);

    // Handle each statement inside
    while( true ) {
        stmt_INTERNAL = NULL;
        rc_INTERNAL = sqlite3_prepare_v2(db_INTERNAL, zSql_INTERNAL, -1, &stmt_INTERNAL, &zLeftover_INTERNAL);

        // Check if we get an empty statement
        if( zSql_INTERNAL == zLeftover_INTERNAL ) {
            break;
        }

        if( rc_INTERNAL != SQLITE_OK ) {
            FATAL("SQLite3: Cannot compile statement %s\nThe error is %s\n", zSql_INTERNAL,
                sqlite3_errmsg(db_INTERNAL));
        }

        if( !stmt_INTERNAL ) {
            // This happens for a comment or white-space;
            zSql_INTERNAL = zLeftover_INTERNAL;
            continue;
        }

        if( sqlite3_column_count(stmt_INTERNAL) != 0 ) {
            WARNING("SQLite3: Statement %s returned a result, but was not supposed to!\n", zSql_INTERNAL);
        }

        // Running the statement
        rc_INTERNAL = sqlite3_step(stmt_INTERNAL);
        if( rc_INTERNAL != SQLITE_DONE && rc_INTERNAL != SQLITE_ROW ) {
            FATAL("SQLite3: Cannot run statement %s\nThe error is %s\n",
                zSql_INTERNAL, sqlite3_errmsg(db_INTERNAL));
        }

        zSql_INTERNAL = zLeftover_INTERNAL;

        sqlite3_finalize(stmt_INTERNAL);
    }
<?php
}

/*
 * Function to deal with parametric queries that do not return anything.
 * There is also a version of this function for scalar return values.
 * $query   : SQL statement with ?1, ?2, ... as the plug in parameters
 * $params  : List of types of the parameters. The size of the list must coincide with
 *      the number of parameters.
 *      Valid types are:
 *          int         (64-bit integer)
 *          int32       (32-bit integer)
 *          double      (64-bit floating point)
 *          text
 */
function sql_statement_parametric_norez( $query, array $params, array $args = [] ) {
    global $__grokit_sqlite_param_list;
    $__grokit_sqlite_param_list = $params;

?>
    zSql_INTERNAL = <?=multiline_string($query)?>;

<?php if( count($args) > 0 ) { ?>
    sprintf(buffer_INTERNAL, zSql_INTERNAL, <?=implode(',', $args)?>);
    zSql_INTERNAL = buffer_INTERNAL;
<?php } ?>

    PDEBUG(zSql_INTERNAL);

    stmt_INTERNAL = NULL;
    rc_INTERNAL = sqlite3_prepare_v2(db_INTERNAL, zSql_INTERNAL, -1, &stmt_INTERNAL, &zLeftover_INTERNAL);

    if( rc_INTERNAL != SQLITE_OK ) {
        FATAL("SQLite3: Cannot compile statement %s\n The error is %s\n",
            zSql_INTERNAL, sqlite3_errmsg(db_INTERNAL));
    }

    if( sqlite3_column_count(stmt_INTERNAL) != 0 ) {
        WARNING("Statement %s\n returned a result when it was not supposed to.\n",
            zSql_INTERNAL);
    }

    // The statement gets run when parameters get instantiated
<?php
}

/*
 * Function to instantiate the parameters of the previously defined parametric statement.
 * It is incorrect to call this outsode of the PARAMETRIC:END-PARAMETRIC construct.
 * $params      : array of values to substitute
 */
function sql_instantiate_parameters( array $params ) {
    global $__grokit_sqlite_param_list;
    if( count($params) != count($__grokit_sqlite_param_list) ) {
        $gotCount = count($params);
        $expectCount = count($__grokit_sqlite_param_list);
        throw new \RuntimeException("Got $gotCount parameters to instantiate but expected $expectCount.");
    }

    $plist = array_combine( $params, $__grokit_sqlite_param_list );

    $cur_index = 1;
?>

        sqlite3_reset(stmt_INTERNAL);
        sqlite3_clear_bindings(stmt_INTERNAL);

<?php foreach( $plist as $name => $type ) {
    switch( $type ) {
    case 'int':
        echo "        sqlite3_bind_int64(stmt_INTERNAL, $cur_index, $name);" . PHP_EOL;
        break;
    case 'int32':
        echo "        sqlite3_bind_int(stmt_INTERNAL, $cur_index, $name);" . PHP_EOL;
        break;
    case 'double':
        echo "        sqlite3_bind_double(stmt_INTERNAL, $cur_index, $name);" . PHP_EOL;
        break;
    case 'text':
        echo "        sqlite3_bind_text(stmt_INTERNAL, $cur_index, $name, -1, SQLITE_TRANSIENT);" . PHP_EOL;
        break;
    default:
        throw new \RuntimeException("Got unknown type $type in parametric SQL query.");
    }

    $cur_index += 1;
} ?>

        // Running the query
        rc_INTERNAL = sqlite3_step(stmt_INTERNAL);
        if( rc_INTERNAL != SQLITE_DONE && rc_INTERNAL != SQLITE_ROW ) {
            FATAL("SQLite3: Cannot run statement %s\n The error is %s\n",
                zSql_INTERNAL, sqlite3_errmsg(db_INTERNAL));
        }
<?php
}

// Ends the parametric definition
function sql_parametric_end() {
    echo '    sqlite3_finalize(stmt_INTERNAL);' . PHP_EOL;

    global $__grokit_sqlite_param_list;
    unset($__grokit_sqlite_param_list);
}

/*
 * Generates code that runs a statement that returns a scalar, and stores that
 * values into the given variable.
 *
 * $query   : the SQL statement
 * $name    : the variable to be defined
 * $type    : the type (compatible with the value returned)
 * $args    : optional prinf parameters
 *
 * The supported types are:
 *      int     (64-bit integer)
 *      int32   (32-bit integer)
 *      double  (64-bit floating point)
 *      text    (equivalent to const char *)
 */
function sql_statement_scalar( $query, $name, $type, array $args = [] ) {
?>

    zSql_INTERNAL = <?=multiline_string($query)?>;

<?php if( count($args) > 0 ) { ?>
    sprintf(buffer_INTERNAL, zSql_INTERNAL, <?=implode(',', $args)?>);
    zSql_INTERNAL = buffer_INTERNAL;
<?php } ?>

    PDEBUG(zSql_INTERNAL);

    stmt_INTERNAL = NULL;
    rc_INTERNAL = sqlite3_prepare_v2(db_INTERNAL, zSql_INTERNAL, -1, &stmt_INTERNAL, &zLeftover_INTERNAL);

    if( rc_INTERNAL != SQLITE_OK ) {
        FATAL("SQLite3: Cannot compile statement %s\n The error is %s\n",
            zSql_INTERNAL, sqlite3_errmsg(db_INTERNAL));
    }

    rc_INTERNAL = sqlite3_step( stmt_INTERNAL );
    if( rc_INTERNAL != SQLITE_ROW ) {
        FATAL("SQLite3: Cannot run statement %s\n The error is %s\n",
            zSql_INTERNAL, sqlite3_errmsg(db_INTERNAL));
    }

    // define the variable with the value
<?php
    switch( $type ) {
        case 'int':
            echo "    long int $name = sqlite3_column_int64(stmt_INTERNAL, 0);" . PHP_EOL;
            break;
        case 'int32':
            echo "    int $name = sqlite3_column_int(stmt_INTERNAL, 0);" . PHP_EOL;
            break;
        case 'double':
            echo "    double $name = sqlite3_column_double(stmt_INTERNAL, 0);" . PHP_EOL;
            break;
        case 'text':
            echo "    const char * $name = (const char *) sqlite3_column_text(stmt_INTERNAL, 0);" . PHP_EOL;
            break;
        default:
            throw new \RuntimeException("Unknown type $type for scalar result of statement.");
    }
?>

    sqlite3_finalize(stmt_INTERNAL);
<?php
}

/*
 * Generates code to handle a SQL query that returns a table.
 * Code to process the rows is inserted after this function call and before the
 * sql_end_statement_table call.
 *
 * $query   : The SQL statement (only one allowed)
 * $params  : Mapping of name => type for the result attributes, in order.
 * $args    : Optional prinf-like arguments for the query.
 *
 * Supported types:
 *      int     (64-bit integer)
 *      int32   (32-bit integer)
 *      double  (64-bit floating point)
 *      text    (equivalent to const char *)
 */
function sql_statement_table( $query, array $params, array $args = [] ) {
?>

    zSql_INTERNAL = <?=multiline_string($query)?>;

<?php if( count($args) > 0 ) { ?>
    sprintf(buffer_INTERNAL, zSql_INTERNAL, <?=implode(',', $args)?>);
    zSql_INTERNAL = buffer_INTERNAL;
<?php } ?>

    PDEBUG(zSql_INTERNAL);

    stmt_INTERNAL = NULL;
    rc_INTERNAL = sqlite3_prepare_v2(db_INTERNAL, zSql_INTERNAL, -1, &stmt_INTERNAL, &zLeftover_INTERNAL);

    if( rc_INTERNAL != SQLITE_OK ) {
        FATAL("SQLite3: Cannot compile statement %s\n The error is %s\n",
            zSql_INTERNAL, sqlite3_errmsg(db_INTERNAL));
    }

    while( true ) {
        // Read a row
        rc_INTERNAL = sqlite3_step(stmt_INTERNAL);
        if( rc_INTERNAL != SQLITE_DONE && rc_INTERNAL != SQLITE_ROW ) {
            FATAL("SQLite3: Cannot run statement %s\n The error is %s\n",
                zSql_INTERNAL, sqlite3_errmsg(db_INTERNAL));
        }

        if( rc_INTERNAL == SQLITE_DONE ) // nothing more to do
            break;

        // Read the columns
<?php $index = 0; ?>
<?php foreach( $params as $name => $type ) {
    switch( $type ) {
    case 'int':
        echo "        long int $name = sqlite3_column_int64(stmt_INTERNAL, $index);" . PHP_EOL;
        break;
    case 'int32':
        echo "        int $name = sqlite3_column_int(stmt_INTERNAL, $index);" . PHP_EOL;
        break;
    case 'double':
        echo "        double $name = sqlite3_column_double(stmt_INTERNAL, $index);" . PHP_EOL;
        break;
    case 'text':
        echo "        const char * $name = (const char *) sqlite3_column_text(stmt_INTERNAL,$index);" . PHP_EOL;
        break;
    default:
        throw new \RuntimeException("Unsupported type $type requested as result table attribute.");
    }

    $index += 1;
} ?>
        // User defined code goes here
<?php
}

/*
 * Ends a previous sql_statement_table()
 */
function sql_end_statement_table() {
?>
    }

    sqlite3_finalize(stmt_INTERNAL);
<?php
}

// Opens a database connection.
// $db      : file containing database. Must already be properly quoted.
function sql_open_database( $db ) {
?>
    sqlite3 *db_INTERNAL;           // database
    int rc_INTERNAL = SQLITE_OK;    // error codes
    sqlite3_stmt* stmt_INTERNAL;    // statement
    char * pzTail_INTERNAL;         // tail pointer
    int nCol_INTERNAL;              // number of columns
    const char *zLeftover_INTERNAL; // leftover for processing multiple statements
    const char * zSql_INTERNAL;
    char buffer_INTERNAL[10000];

    rc_INTERNAL = sqlite3_open(<?=$db?>, &db_INTERNAL);
    if( rc_INTERNAL != SQLITE_OK ) {
        FATAL("Cannot open database: %s\n", sqlite3_errmsg(db_INTERNAL));
    }

    // start a transaction to make sure the code runs faster
<?php sql_statements_norez('"BEGIN TRANSACTION;"'); ?>
#ifdef DEBUG
    printf("\nBEGIN TRANSACTION;");
#endif // DEBUG
<?php
}

// Gives access to the database object so that it can be passed to another
// code snippet.
function sql_database_object() {
    echo 'db_INTERNAL';
}

function sql_existing_database( $db ) {
?>
    sqlite3 *db_INTERNAL = <?=$db?>; // database
    int rc_INTERNAL = SQLITE_OK;    // error codes
    sqlite3_stmt* stmt_INTERNAL;    //statement
    char* pzTail_INTERNAL;          // tail pointer
    int nCol_INTERNAL;              // number of columns
    const char *zLeftover_INTERNAL; // leftover for processing multiple statements
    const char *zSql_INTERNAL;
    char buffer_INTERNAL[10000];
<?php
}

function sql_close_database() {
    sql_statements_norez('"END TRANSACTION;"');
?>

#ifdef DEBUG
    printf("\nEND TRANSACTION;");
#endif // DEBUG

    sqlite3_close(db_INTERNAL);
<?php
}
?>

#include <sqlite3.h>
#include <assert.h>
#include "Debug.h"
#include "Errors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
