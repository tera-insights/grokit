<?
// Copyright 2013 Tera insights, LLC. All Rights Reserved.

require_once('DataFunctions.php');
?>

/** Data Structures for Service Waypoints */

#ifndef _SERVICE_DATA_H_
#define _SERVICE_DATA_H_

#include <string>

#include "SerializeJson.h"

<?
grokit\create_base_data_type(
    "ServiceData",
    "DataC",
    [ 'id' => 'std::string', 'service' => 'std::string', 'kind' => 'std::string', 'data' => 'Json::Value' ],
    [ ],
    true
);
?>

namespace ServiceErrors {
    using type = Json::Int;

    constexpr type NoSuchService = 100;
    constexpr type NoSuchOperation = 101;

    constexpr type OperationNotSupported = 200;

    inline
    ServiceData MakeError( ServiceData& initialRequest, type error, std::string message, Json::Value data = Json::Value(Json::nullValue) ) {
        Json::Value errData(Json::objectValue);
        errData["code"] = error;
        errData["message"] = message;
        if( !data.isNull() ) {
            errData["data"] = data;
        }

        ServiceData ret(initialRequest.get_id(), initialRequest.get_service(), "error", errData);
        return ret;
    }
}

#endif // _SERVICE_DATA_H_
