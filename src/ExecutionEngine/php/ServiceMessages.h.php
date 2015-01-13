<?
// Copyright 2013 Tera Insights, LLC. All Rights Reserved.

require_once('MessagesFunctions.php');
?>

#ifndef SERVICE_MESSAGES_H_
#define SERVICE_MESSAGES_H_

#include <string>

#include "ServiceData.h"

// Message containing a request for a service to perform some work.
<?
grokit\create_message_type(
    "ServiceRequestMessage",
    [  ],
    [ 'request' => 'ServiceData' ],
    true
);
?>

/*
 * Message containing an informational message regarding a service as a whole.
 * Valid values of "status":
 *
 *  "started"       The service has been started, but is not yet ready to begin
 *                  processing requests. Any requests received will be queued.
 *  "ready"         The service is ready to begin processing requests.
 *  "stopping"      The service is now stopping, and will reject any further
 *                  requests. Any requests still being processed will be finished.
 *  "stopped"       The service has completely stopped, requests are being processed
 *                  and no requests will be accepted.
 */
<?
grokit\create_message_type(
    "ServiceInfoMessage",
    [ 'service' => 'std::string', 'status' => 'std::string', 'data' => 'Json::Value' ],
    [ ],
    true
);
?>

<?
grokit\create_message_type(
    "ServiceControlMessage",
    [ ],
    [ 'control' => 'ServiceData' ],
    true
);
?>

/*
 * Message containing a reply to a request. There are many kinds of replies, indicated
 * by the "kind" value:
 *
 *  "received"      The request has been successfully received and has been queued for
 *                  processing.
 *  "processing"    The request is now being processed
 *  "result"        This message contains a result produced in response to the request.
 *  "error"         The request was unable to be serviced for some reason, and
 *                  information about the error will be available in "data"
 */
<?
grokit\create_message_type(
    "ServiceReplyMessage",
    [ ],
    [ 'reply' => 'ServiceData' ],
    true
);
?>

#endif // SERVICE_MESSAGES_H_
