<?
//  Copyright 2013 Tera Insights, LLC. All Rights Reserved.

require_once('MessagesFunctions.php');
?>

#ifndef _CATALOG_MESSAGES_H_
#define _CATALOG_MESSAGES_H_

#include "SerializeJson.h"

<?
grokit\create_message_type(
    'CatalogUpdateMessage',
    [ ],
    [ 'catalog' => 'Json::Value' ],
    true
);
?>

#endif //_CATALOG_MESSAGES_H_
