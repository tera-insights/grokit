#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "FileMetadata.h"
#include "Errors.h"
#include "MetadataDB.h"
#include "DiskArray.h"
#include "Constants.h"
#include <set>

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

using namespace std;

// ======================== FileMetadata class definitions ===================

FileMetadata::FileMetadata(const char *_relName, int _numCols):
    relName(NULL),
    relID(-1),
    newRelation(false),
    numCols(0),
    numChunks(0),
    modified(false),
    chunkMetaD(),
    chkFilled(-1),
    colsFilled(0)
{
    relName = strdup(_relName);
    newRelation=false; // we change it latter if not found

<?php
grokit\sql_open_database( 'GetMetadataDB() ' );
?>
;

    // Create all tables we need if not already existing
<?php
grokit\sql_statements_norez( <<<'EOT'
"
    /* Relations */
    CREATE TABLE IF NOT EXISTS Relations (
      relID          INTEGER,
      arrayID        INTEGER,
            relName        TEXT,
            numColumns     INTEGER,
            freeChunkId    INTEGER
    );

    /* Chunks */
    CREATE TABLE IF NOT EXISTS Chunks (
      /* numbering starts at 0 */
      chunkID        INTEGER NOT NULL,
      relID          INTEGER NOT NULL,
      numTuples      INTEGER NOT NULL,
      clusterMin     INTEGER DEFAULT 1,
      clusterMax     INTEGER DEFAULT 0
    );

    /* Columns */
    CREATE TABLE IF NOT EXISTS Columns (
      colNo          INTEGER NOT NULL,
      relID          INTEGER NOT NULL,
      chunkID        INTEGER NOT NULL,
      startPage      INTEGER NOT NULL,
      sizePages      INTEGER NOT NULL,
      sizeBytes      INTEGER NOT NULL,
      startPageCompr INTEGER NOT NULL,
      sizePagesCompr INTEGER NOT NULL,
      sizeBytesCompr INTEGER NOT NULL
    );

    /* Fragment */
    CREATE TABLE IF NOT EXISTS Fragment(
            relID           INTEGER,
            chunkID         INTEGER,
            colNo           INTEGER,
      endPos          INTEGER
    );

    /* FragmentTuples */
    CREATE TABLE IF NOT EXISTS FragmentTuples(
      tuples        INTEGER, /* number of tuples */
            relID           INTEGER,
            chunkID         INTEGER
    );

"
EOT
, [ ] );
?>
;

    // first decide if this relation is new or old
<?php
grokit\sql_statement_scalar( <<<'EOT'
"
        SELECT COUNT(*)
        FROM Relations
        WHERE relName='%s';
"
EOT
, 'haveRel', 'int', [ 'relName', ]);
?>
;

    if (haveRel == 1 ) { // existing relation

<?php
grokit\sql_statement_table( <<<'EOT'
"
        SELECT relID, numColumns, freeChunkID
        FROM Relations
          WHERE relName='%s';
"
EOT
, [ '_relID_' => 'int', '_numCols_' => 'int', '_numChunks_' => 'int', ], [ 'relName', ] );
?>
{
            relID = _relID_;
            numCols = _numCols_;
            numChunks = _numChunks_;
        }<?php
grokit\sql_end_statement_table();
?>
;

        FATALIF( numCols != _numCols, "Did not indicate the correct number of columns"
                         "\nProbably trying to write the wrong relation\n");

        chunkMetaD.resize(numChunks);

        // fill datastructures for all chunks
<?php
grokit\sql_statement_table( <<<'EOT'
"
      SELECT chunkID, numTuples, clusterMin, clusterMax
        FROM Chunks
          WHERE relID=%d;
    "
EOT
, [ '_chunkID' => 'int', '_numTuples' => 'int', 'clusterMin' => 'int', 'clusterMax' => 'int', ], [ 'relID', ] );
?>
{
            FATALIF( _chunkID >= numChunks, "Got more chunks than specified in Relation");
            chunkMetaD[_chunkID].Initialize(numCols, _numTuples, clusterMin, clusterMax);
        }<?php
grokit\sql_end_statement_table();
?>
;

        // fill datastructures for all columns
        // for each column, populate the datastructures
<?php
grokit\sql_statement_table( <<<'EOT'
"
          SELECT chunkID, colNo, startPage, sizePages, sizeBytes, startPageCompr, sizePagesCompr, sizeBytesCompr
        FROM Columns
        WHERE relID=%d;
    "
EOT
, [ '_chunkID2' => 'int', '_colNo' => 'int', '_startPage' => 'int', '_sizePages' => 'int', '_sizeBytes' => 'int', '_startPageCompr' => 'int', '_sizePagesCompr' => 'int', '_sizeBytesCompr' => 'int', ], [ 'relID', ] );
?>
{
            chunkMetaD[_chunkID2].colMetaData[_colNo].Initialize(_startPage, _sizePages, _sizeBytes,
                                                                                                                     _startPageCompr, _sizePagesCompr,
                                                                                                                     _sizeBytesCompr);
        }<?php
grokit\sql_end_statement_table();
?>
;

<?php
grokit\sql_statement_table( <<<'EOT'
"
      SELECT chunkID, colNo, endPos
      FROM Fragment
        WHERE relID=%d
      ORDER BY chunkID, colNo, endPos;
    "
EOT
, [ '_chunkID3' => 'int', '_colNo2' => 'int', 'endPos' => 'int', ], [ 'relID', ] );
?>
{
            chunkMetaD[_chunkID3].colMetaData[_colNo2].fragments.Initialize(endPos);
        }<?php
grokit\sql_end_statement_table();
?>
;

    } else { // new relation
      newRelation=true;
      numChunks = 0;
      numCols = _numCols;


      // get a new relation id
<?php
grokit\sql_statement_scalar( <<<'EOT'
"
        SELECT MAX(relID)
          FROM Relations;
    "
EOT
, '_relID', 'int', [ ]);
?>
;

      relID=_relID+1; // new id


    }

    // close the database
<?php
grokit\sql_close_database();
?>
;
}

void FileMetadata::Print(void){
  cout << "\nChunk information";
  for (int chunkit = 0; chunkit < chunkMetaD.size(); chunkit++) {
    printf("\n Chunkit = %d,   relID = %d,    Numtuples = %d", chunkit, relID, chunkMetaD[chunkit].numTuples);
  }

  for (int chunkit = 0; chunkit < chunkMetaD.size(); chunkit++) {
    for (int colit = 0; colit < chunkMetaD[chunkit].colMetaData.size(); colit++) {
      printf("\n Chunkit = %d, colit = %d, relID = %d, Startpage = %d, size pages = %d, sizeBytes = %d, startpageCompr = %d, sizePageCompr = %d, sizeBytesCompr = %d",
         chunkit, colit, relID, chunkMetaD[chunkit].colMetaData[colit].startPage, chunkMetaD[chunkit].colMetaData[colit].sizePages, chunkMetaD[chunkit].colMetaData[colit].sizeBytes, chunkMetaD[chunkit].colMetaData[colit].startPageCompr, chunkMetaD[chunkit].colMetaData[colit].sizePagesCompr, chunkMetaD[chunkit].colMetaData[colit].sizeBytesCompr); fflush(stdout);
    }
  }
}

//>

void FileMetadata::DeleteRelation(std::string name) {
<?php
grokit\sql_open_database( 'GetMetadataDB() ' );
?>

    int relID;
    const char * relName = name.c_str();
<?php
grokit\sql_statement_scalar( <<<'EOT'
"
        SELECT COUNT(*)
        FROM Relations
        WHERE relName='%s';
"
EOT
, 'haveRel', 'int', [ 'relName', ]);
?>
;

    if (haveRel == 1 ) { // existing relation

<?php
grokit\sql_statement_table( <<<'EOT'
"
        SELECT relID
        FROM Relations
          WHERE relName='%s';
"
EOT
, [ '_relID_' => 'int', ], [ 'relName', ] );
?>
{
            relID = _relID_;
        }<?php
grokit\sql_end_statement_table();
?>
;
        DeleteContentSQL(relID, <?=grokit\sql_database_object()?>);

<?
grokit\sql_statements_norez( <<<'EOT'
"
    DELETE FROM Relations
    WHERE relID=%d;
"
EOT
, [ 'relID' ])
?>
    } else {
        WARNING("Attempting to delete unknown relation %s", relName);
    }

<?php
grokit\sql_close_database();
?>
}

bool FileMetadata::DeleteContent(sqlite3 * db) {
    if( modified )
        return false;

    bool closeDB = true;

    if( db == nullptr ) {
<?php
grokit\sql_open_database( 'GetMetadataDB() ' );
?>

        chunkMetaD.clear();
        numChunks = 0;
        DeleteContentSQL(relID, <?=grokit\sql_database_object()?>);
<?php
grokit\sql_statements_norez( <<<'EOT'
"
        UPDATE Relations SET freeChunkID=%ld
        WHERE relID=%d;
"
EOT
, [ 'numChunks', 'relID', ] );
?>
;
        DiskArray& diskArray = DiskArray::GetDiskArray();
        diskArray.DeleteRelationSpace(relID);
        diskArray.Flush(db);
<?php
grokit\sql_close_database();
?>
    } else {
<?
grokit\sql_existing_database('db');
?>

        chunkMetaD.clear();
        numChunks = 0;
        DeleteContentSQL(relID, <?=grokit\sql_database_object()?>);
        // update the freeChunkID of the relation
<?php
grokit\sql_statements_norez( <<<'EOT'
"
        UPDATE Relations SET freeChunkID=%ld
        WHERE relID=%d;
"
EOT
, [ 'numChunks', 'relID', ] );
?>
;
        DiskArray& diskArray = DiskArray::GetDiskArray();
        diskArray.DeleteRelationSpace(relID);
        diskArray.Flush(db);
    }

    return true;
}

void FileMetadata::DeleteContentSQL(int relID, sqlite3 * db) {
    // delete all previous info on this relation
<?  grokit\sql_existing_database( 'db' ); ?>

<?php
grokit\sql_statements_norez( <<<'EOT'
"
        DELETE FROM Chunks
        WHERE relID=%d;
"
EOT
, [ 'relID', ] );
?>
;

<?php
grokit\sql_statements_norez( <<<'EOT'
"
        DELETE FROM Columns
        WHERE relID=%d;
"
EOT
, [ 'relID', ] );
?>
;

<?php
grokit\sql_statements_norez( <<<'EOT'
"
        DELETE FROM Fragment
        WHERE relID=%d;
"
EOT
, [ 'relID', ] );
?>
;

<?php
grokit\sql_statements_norez( <<<'EOT'
"
        DELETE FROM FragmentTuples
        WHERE relID=%d;
"
EOT
, [ 'relID', ] );
?>
;

}

void FileMetadata::Flush(void) {
    // to write the informatinon we use write system call
    // since it allows any amout of information to be writen
    if (!modified)
        return;

    PDEBUG("FileMetadata::Flush()");

#ifdef DEBUG_FILEMETADATA
    Print();
#endif

<?php
grokit\sql_open_database( 'GetMetadataDB() ' );
?>
;

    // if relation is new, first write the entry in relations
    if (newRelation){
      cout << "CREATING A NEW RELATION" << endl;

      DiskArray& diskArray = DiskArray::GetDiskArray();
      int arrayID = diskArray.getArrayID();

<?php
grokit\sql_statements_norez( <<<'EOT'
"
          INSERT INTO Relations(relID, arrayID, relName, numColumns, freeChunkID)
        VALUES (%d, %d, '%s', %d, 0);
      "
EOT
, [ 'relID', 'arrayID', 'relName', 'numCols', ] );
?>
;
    }

    // update the freeChunkID of the relation
<?php
grokit\sql_statements_norez( <<<'EOT'
"
        UPDATE Relations SET freeChunkID=%ld
        WHERE relName='%s';
"
EOT
, [ '(unsigned long)numChunks', 'relName', ] );
?>
;

    DeleteContentSQL(relID, <?=grokit\sql_database_object()?>);

    // Now flush all chunk info first
<?php
grokit\sql_statement_parametric_norez( <<<'EOT'
"
        INSERT INTO Chunks(chunkID, relID, numTuples, clusterMin, clusterMax)
        VALUES (?1, ?2, ?3, ?4, ?5);
"
EOT
, [ 'int', 'int', 'int', 'int', 'int', ], [ ]);
?>
;
            for (int chunkit = 0; chunkit < chunkMetaD.size(); chunkit++) {
              ChunkMetaD::ClusterRange cRange = chunkMetaD[chunkit].getClusterRange();
<?php
grokit\sql_instantiate_parameters([
  'chunkit', 'relID', 'chunkMetaD[chunkit].numTuples',
  'cRange.first', 'cRange.second']);
?>
;
            }
<?php
grokit\sql_parametric_end();
?>
;

    // Now flush all column info
<?php
grokit\sql_statement_parametric_norez( <<<'EOT'
"
        INSERT INTO Columns(colNo, chunkID, relID, startPage, sizePages, sizeBytes, startPageCompr, sizePagesCompr, sizeBytesCompr)
        VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9);
        "
EOT
, [ 'int', 'int', 'int', 'int', 'int', 'int', 'int', 'int', 'int', ], [ ]);
?>
;
            for (int chunkit = 0; chunkit < chunkMetaD.size(); chunkit++) {
                for (int colit = 0; colit < chunkMetaD[chunkit].colMetaData.size(); colit++) {
<?php
grokit\sql_instantiate_parameters( [ 'colit', 'chunkit', 'relID', 'chunkMetaD[chunkit].colMetaData[colit].startPage', 'chunkMetaD[chunkit].colMetaData[colit].sizePages', 'chunkMetaD[chunkit].colMetaData[colit].sizeBytes', 'chunkMetaD[chunkit].colMetaData[colit].startPageCompr', 'chunkMetaD[chunkit].colMetaData[colit].sizePagesCompr', 'chunkMetaD[chunkit].colMetaData[colit].sizeBytesCompr', ] );
?>
;
                }
            }
<?php
grokit\sql_parametric_end();
?>
;

    // Now flush all fragment info for each column
<?php
grokit\sql_statement_parametric_norez( <<<'EOT'
"
            INSERT INTO Fragment(relID, chunkID, colNo, endPos) VALUES (?1, ?2, ?3, ?4);
            "
EOT
, [ 'int', 'int', 'int', 'int', ], [ ]);
?>
;
            for (int chunkit = 0; chunkit < chunkMetaD.size(); chunkit++) {
                for (int colit = 0; colit < chunkMetaD[chunkit].colMetaData.size(); colit++) {
                    // iterate through ranges
                    for (int i = 0; i < chunkMetaD[chunkit].colMetaData[colit].fragments.startPositions.size(); i++) {
<?php
grokit\sql_instantiate_parameters( [ 'relID', 'chunkit', 'colit', 'chunkMetaD[chunkit].colMetaData[colit].fragments.startPositions[i]', ] );
?>
;
                    }
                }
            }
<?php
grokit\sql_parametric_end();
?>
;

    // now ask the diskArray to flush as well
    DiskArray& diskArray = DiskArray::GetDiskArray();
    diskArray.Flush(<?=grokit\sql_database_object()?>);

    // and that is about all
<?php
grokit\sql_close_database();
?>
;

    modified=false;
}
