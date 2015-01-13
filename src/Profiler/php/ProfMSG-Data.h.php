#ifndef _PROF_MSG_DATA_H_
#define _PROF_MSG_DATA_H_

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

#include <inttypes.h>
#include <string>

#include "Data.h"
#include "TwoWayList.h"

/** Container for counters

*/
<?php
grokit\create_data_type( "PCounter",
    "DataC",
    [ 'name' => 'std::string', 'value' => 'int64_t', 'group' => 'std::string', ],
    [ ],
    true
);
?>

typedef TwoWayList <PCounter> PCounterList;

#endif // _PROF_MSG_DATA_H_
