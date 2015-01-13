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
require_once('DataFunctions.php');
?>



#ifndef DISK_IO_DATA_
#define DISK_IO_DATA_

#include "Data.h"
#include "TwoWayList.h"
#include "Column.h"
#include "EEExternMessages.h"

// the base class of the hierarchy
/** The common parts are:
		
		startPage: the starting page on the disk where data is read/written
		sizePages: the size of the column in pages
*/
		
<?php
grokit\create_base_data_type( "DiskData", "Data", [ 'startPage' => 'off_t', 'sizePages' => 'off_t', ], [ ] );
?>



/** Used by the ChunkReaderWriter to get a description of 
		the column content to write on the disk. 

    Arguments:
		  slot: which column is this about
			sizePages:  size in number of pages
			startPage: the start of the column on disk

			Sizes in bytes:

			sizeCompressed: if !=0,  the column is compressed
			sizeUncompressed;
			
*/

<?php
grokit\create_data_type( "ColReadDescription", "DiskData", [ 'slot' => 'int', 'sizeCompressed' => 'off_t', 'sizeUncompressed' => 'off_t', ], [ ] );
?>


typedef TwoWayList< ColReadDescription > ColReadDescContainer;

/** Used by ChunkReaderWriter to get instruction of what columns to
    write

		Arguments:
		  	column: the column to be written
				off_t startPageCompr; // the first page where the compressed part goes
				off_t sizePagesCompr; // the size, in pages of the compressed part

*/

<?php
grokit\create_data_type( "ColWriteDescription", "DiskData", [ 'startPageCompr' => 'off_t', 'sizePagesCompr' => 'off_t', ], [ 'column' => 'Column', ] );
?>


typedef TwoWayList< ColWriteDescription > ColWriteDescContainer;


/** These are requests going to the disk. The pageSize is the MMap page
		The requests are sent to the DiskArray who sends them to the HDThreads.

		Arguments:
		  memLoc: the memory location where the read/write is done

*/
<?php
grokit\create_data_type( "DiskRequestData", "DiskData", [ 'memLoc' => 'void*', ], [ ] );
?>


typedef TwoWayList< DiskRequestData > DiskRequestDataContainer;
		
/** Internal datatype used by the ChunkReaderWriter to keep track of requests
		
		Arguments:
				chunkID: id of the chunk we are dealing with
				hMsg: the hopping message to send back to EE
				token: the token to send back
*/
<?php
grokit\create_data_type( "CRWRequest", "Data", [ 'chunkID' => 'off_t', ], [ 'hMsg' => 'HoppingDataMsg', 'token' => 'GenericWorkToken', ] );
?>


typedef Keyify<off_t> KOff_t; // keyified off_t
typedef EfficientMap< KOff_t, CRWRequest > IDToRequestMap;

#endif // DISK_IO_DATA_
