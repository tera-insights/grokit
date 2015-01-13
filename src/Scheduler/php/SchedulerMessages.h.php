<?php
//
//  Copyright 2013 Tera Insights LLC. All Rights Reserved.
//  Author: Christopher Dudley
require_once('MessagesFunctions.php')
?>

#ifndef _SCHEDULER_MESSAGES_H_
#define _SCHEDULER_MESSAGES_H_

#include "TimePoint.h"

/*********** Single-Run Scheduler Events **********/

// Register a timer that does not repeat and expires `delay`
// after the timer is registered
<?
grokit\create_message_type(
    'SchedulerRegisterSingleRelative',
    [ 'delay' => 'TimePoint' ],
    [ 'dest' => 'EventProcessor', 'payload' => 'Data' ]
);
?>

// Register a timer that does not repeat and expires at the specified
// time
<?
grokit\create_message_type(
    'SchedulerRegisterSingleAbsolute',
    [ 'time' => 'TimePoint' ],
    [ 'dest' => 'EventProcessor', 'payload' => 'Data' ]
);
?>

#endif // _SCHEDULER_MESSAGES_H_
