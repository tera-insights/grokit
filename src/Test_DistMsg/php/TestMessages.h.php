<?php

// Copyright 2013 Tera Insights, LLC. All Rights Reserved.

require_once 'MessagesFunctions.php'

?>

#ifndef _TEST_MESSAGES_H_
#define _TEST_MESSAGES_H_

<?php
grokit\create_message_type( 'TestMessage', [ 'id' => 'int' ], [ 'msg' => 'Json::Value' ], true );
?>

#endif // _TEST_MESSAGES_H_
