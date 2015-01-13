<?php
//
//  Copyright 2013 Tera Insights LLC. All Rights Reserved.
//  Author: Christopher Dudley
require_once('MessagesFunctions.php')
?>

#ifndef _SCHEDULER_CLOCK_MESSAGES_H_
#define _SCHEDULER_CLOCK_MESSAGES_H_

/*********** Common Messages **********/

<?
grokit\create_message_type(
    'SchedulerHeartbeat',
    [ ],
    [ ]
);
?>

<?
grokit\create_message_type(
    'SchedulerClockRegister',
    [ ]
);
?>

#endif // _SCHEDULER_CLOCK_MESSAGES_H_
