/** The methods in DiskArrayImp that are partially implemented in SQL

        The DiskArray metadata is assumed already instantiated (no code to
        create it here). A tool, called diskArrayInit can create an
        initial state of the data.
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
require_once('SQLite.php');
?>



#include "DiskArrayImp.h"
#include "Errors.h"
#include "MetadataDB.h"
#include "Constants.h"
#include "MmapAllocator.h"

#include <iostream>
#include <pthread.h>
#include <random>

using namespace std;

DiskArrayImp::DiskArrayImp(bool isReadOnly)
#ifdef  DEBUG_EVPROC
    :EventProcessorImp(true, "DiskArrayImp")
#endif
{
    //initialize the mutex
    pthread_mutex_init(&lock, NULL);

    <?php
grokit\sql_open_database( 'GetMetadataDB() ' );
?>
;

    // Just in case the disk arrays do not exist
    <?php
grokit\sql_statements_norez( <<<'EOT'
"
      CREATE TABLE  IF NOT EXISTS DiskArrays (
          /* name must be "Default" for the default array */
           name                                 TEXT,
           arrayID                              INTEGER                   PRIMARY KEY,
           arrayHash                            INTEGER                   NOT NULL,
           pageMultExp                          INTEGER                   NOT NULL,
           stripeParam1                         INTEGER                   NOT NULL,
           stripeParam2                         INTEGER                   NOT NULL,
           numberOfPages                        INTEGER                   NOT NULL
      );

      CREATE TABLE IF NOT EXISTS Stripes (
          /* diskID is INT not INTEGER since it cannot be 0 */
          /* arrayID is a foreign key to DiskArrays table */
           diskID                        INT,
           arrayID                       INTEGER,
           fileName                      TEXT
      );
      "
EOT
, [ ] );
?>
;

    meta.arrayID = -1; // invalid. If the next piece code does not set it to valid
    // latter code will fail

    // read the info from DiskArray
    <?php
grokit\sql_statement_table( <<<'EOT'
"
        SELECT arrayID, arrayHash, pageMultExp, stripeParam1, stripeParam2, numberOfPages
        FROM DiskArrays
        WHERE name='Default';
"
EOT
, [ 'arrayID' => 'int', 'arrayHash' => 'int', 'pageMultExp' => 'int', 'stripeParam1' => 'int', 'stripeParam2' => 'int', 'numberOfPages' => 'int', ], [ ] );
?>
{
        meta.arrayID = arrayID;
         meta.arrayHash = arrayHash;
        meta.pageMultExp = pageMultExp;
        meta.stripeParam1 = stripeParam1;
        meta.stripeParam2 = stripeParam2;
        meta.numberOfPages = numberOfPages;
    }<?php
grokit\sql_end_statement_table();
?>
;

    if (meta.arrayID == -1){
        // no disk array defined yet. Must interact with the user
        printf("No Disk Array defined yet. Please answer the following quesions so we can set one up\n");
        meta.arrayID = 1; // default
        printf("What is the page muliplier exponent of the array? The disk requests will be (2 ^ pmultExp) * %d bytes\n", MMAP_PAGE_SIZE);
        cin >> meta.pageMultExp;
        FATALIF(meta.pageMultExp > 8, "Now, now, is that not excessive?");

        std::random_device rd;
        std::mt19937_64 rand_gen(rd());
        meta.arrayHash = rand_gen(); // random arrayHash
        meta.stripeParam1 = rand_gen();
        meta.stripeParam2 = rand_gen();
        meta.numberOfPages = 0;

        <?php
grokit\sql_statements_norez( <<<'EOT'
"
          INSERT INTO DiskArrays(name, arrayID, arrayHash, pageMultExp, stripeParam1, stripeParam2, numberOfPages)
          VALUES ('Default', %ld, %ld, %ld, %ld, %ld, %ld);
"
EOT
, [ 'meta.arrayID', 'meta.arrayHash', 'meta.pageMultExp', 'meta.stripeParam1', 'meta.stripeParam2', 'meta.numberOfPages', ] );
?>
;

        printf("The disk array can use multiple stripes. How many stripes should we use?\n");
        cin >> meta.HDNo;
        FATALIF( meta.HDNo<1 || meta.HDNo > 1000, "The number of disks is unacceptable");

        printf("Pattern for the stripes fies. Use %%d for the position of the numbers. The numbering starts at 1\n");
        string pattern;
        cin >> pattern;

        <?php
grokit\sql_statement_parametric_norez( <<<'EOT'
"
          INSERT INTO Stripes(diskID, arrayID, fileName) VALUES (?1, ?2, ?3);
          "
EOT
, [ 'int', 'int', 'text', ], [ ]);
?>
;
        for (int i=0; i<meta.HDNo; i++){
            char fileName[1000];
            sprintf(fileName, pattern.c_str(), i+1);
	    HDThread::CreateStripe(fileName, meta.arrayHash, i, MMAP_PAGE_SIZE << meta.pageMultExp);

            <?php
grokit\sql_instantiate_parameters( [ 'i', 'meta.arrayID', 'fileName', ] );
?>
;
        }
        <?php
grokit\sql_parametric_end();
?>
;

    } else { // disk array is valid
#if 0
        printf("Opening Disk Array %ld: pageMultExp:%ld,\tnumberOfPages=%ld\n", meta.arrayID, meta.pageMultExp, meta.numberOfPages);
#endif
        // get the number of drives
        <?php
grokit\sql_statement_scalar( <<<'EOT'
"
          SELECT COUNT(diskID)
          FROM Stripes
          WHERE arrayID=%d;
      "
EOT
, '_cnt', 'int', [ 'meta.arrayID', ]);
?>
;
        meta.HDNo = _cnt;
    }

    totalPages = 0;
    hds = new EventProcessor[meta.HDNo];

    // read the stripes from Stripes and start the HD threads
    <?php
grokit\sql_statement_table( <<<'EOT'
"
        SELECT fileName
        FROM Stripes
        WHERE arrayID=%d;
    "
EOT
, [ 'fileName' => 'text', ], [ 'meta.arrayID', ] );
?>
{
        HDThread hd(fileName, meta.arrayHash, myInterface, DISK_OPERATION_STATISTICS_INTERVAL, isReadOnly);
	int diskID = hd.DiskNo();
        hd.ForkAndSpin();
	FATALIF( hds[diskID].IsValid(), "Stripe %d already initized", diskID);
        hds[diskID].swap(hd); // put the hd in the vector

    }<?php
grokit\sql_end_statement_table();
?>
;

    // check that all the stripes are correctly initialized
    for (int i=0; i<meta.HDNo; i++){
    	FATALIF( !hds[i].IsValid(), "Stripe %d is not initized", i);
    }

    // now we let the space manager initialize itself
    diskSpaceMng.SetArrayID(meta.arrayID);
    diskSpaceMng.Load(<?=grokit\sql_database_object()?>);
    <?php
grokit\sql_close_database();
?>
;

    //priority for processing read chunks is higher than accepting new chunks
    RegisterMessageProcessor(DiskStatistics::type, &ProcessDiskStatistics, 2);
    RegisterMessageProcessor(DiskOperation::type, &DoDiskOperation, 1);
}

void DiskArrayImp::Flush(void) {
    // the Array did not change, just the space manager
    <?php
grokit\sql_open_database( 'GetMetadataDB() ' );
?>
;
    printf("\nDiskArrayImp::Flush We open the DB");
    diskSpaceMng.Flush(<?=grokit\sql_database_object()?>);
    printf("\nDiskArrayImp::Flush We close the DB");
    <?php
grokit\sql_close_database();
?>
;
}
