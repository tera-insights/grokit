
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



#ifndef TOKENS_H
#define TOKENS_H

// this file contains all of the various work token types

#include "Data.h"
#include "DistributedQueue.cc"

// this macro defines the generic token type, which is sent to waypoints when they
// request the ability to do some work... tokens must also be sent back to the
// execution engine when the work is done.  grantedTo is the identifier for the
// waypoint who was actually granted the right to use the token
<?php
grokit\create_base_data_type( "GenericWorkToken", "Data", [ 'label' => 'int', ], [ ] );
?>


// this macro defines the CPU token type
<?php
grokit\create_data_type( "CPUWorkToken", "GenericWorkToken", [ ], [ ] );
?>


// this macro defines the Disk token type
<?php
grokit\create_data_type( "DiskWorkToken", "GenericWorkToken", [ ], [ ] );
?>


// this is used to send a list of disk tokens for writer waypoints to use
typedef DistributedQueue <DiskWorkToken> DiskWorkTokenQueue;

#endif
