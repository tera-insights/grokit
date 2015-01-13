
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



#ifndef _PROF_MSG_H_
#define _PROF_MSG_H_

#include "ProfMSG-Data.h"
#include "Message.h"
#include "Timer.h"
#include <ctime>
#include <cinttypes>

// messages to the profiler to record execution

// message with basic counter to be incremented
<?php
grokit\create_message_type(
    'ProfileMessage',
    [ 'wallStart' => 'int64_t', 'wallEnd' => 'int64_t' ],
    [ 'counter' => 'PCounter', ],
    true,
    true
);
?>

// message with a set of counters
<?php
grokit\create_message_type(
    'ProfileSetMessage',
    [ 'wallStart' => 'int64_t', 'wallEnd' => 'int64_t', ],
    [ 'counters' => 'PCounterList', ],
    true,
    true
);
?>

// Message with no duration
<?php
grokit\create_message_type(
    'ProfileInstantMessage',
    [ 'wallTime' => 'int64_t' ],
    [ 'counter' => 'PCounter' ],
    true,
    true
);
?>

// Message with a single progress counter
<?
grokit\create_message_type(
    'ProfileProgressMessage',
    [ 'wallTime' => 'int64_t',  ],
    [ 'counter' => 'PCounter',  ],
    true,
    true
);
?>

// Message with multiple progress counters
<?
grokit\create_message_type(
    'ProfileProgressSetMessage',
    [ 'wallTime' => 'int64_t',  ],
    [ 'counters' => 'PCounterList', ],
    true,
    true
);
?>

// message stating that the current time interval has elapsed
<?php
grokit\create_message_type(
    'ProfileIntervalMessage',
    [ 'wallTime' => 'int64_t', 'cTime' => 'int64_t', 'cTime' => 'int64_t' ],
    [ 'statInfo' => 'Json::Value' ]
);
?>

// message sent by the profiler to the frontend, holding aggregate information about
// performance counters for the previous second.
// times are in milliseconds since the epoch

<?php
grokit\create_message_type(
    'ProfileAggregateMessage',
    [ 'wallStart' => 'int64_t', 'wallEnd' => 'int64_t' ],
    [ 'counters' => 'Json::Value', 'progress' => 'Json::Value' ],
    true
);
?>

#endif //  _PROF_MSG_H_
