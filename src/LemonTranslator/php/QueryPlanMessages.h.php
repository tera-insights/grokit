<?
// Copyright 2013 Tera Insights, LLC. All Rights Reserved.
// Author: Christopher Dudley

require_once('MessagesFunctions.php');
?>

#ifndef _QUERY_PLAN_MESSAGES_H_
#define _QUERY_PLAN_MESSAGES_H_

#include "SerializeJson.h"

<?
grokit\create_message_type('QueryPlanMessage',
    [ ],
    [ 'info' => 'Json::Value' ],
    true
);
?>

#endif // _QUERY_PLAN_MESSAGES_H_
