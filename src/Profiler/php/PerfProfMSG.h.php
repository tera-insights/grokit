<?
// Copyright 2013 Tera Insights, LLC. All Rights Reserved.

require_once('MessagesFunctions.php');
?>

#ifndef _PERF_PROF_MSG_H_
#define _PERF_PROF_MSG_H_

#include "SerializeJson.h"
#include <cinttypes>

<?
grokit\create_message_type(
    'PerfTopMessage',
    [ 'wallTime' => 'int64_t' ],
    [ 'info' => 'Json::Value' ],
    true
);
?>

#endif // _PERF_PROF_MSG_H_
