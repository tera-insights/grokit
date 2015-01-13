/**
	This header file contains the messages used by CodeLoader.
*/

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



#ifndef _CL_MESSAGES_H_
#define _CL_MESSAGES_H_

#include <string>

#include "Message.h"
#include "SymbolicWaypointConfig.h"
#include "WayPointConfigureData.h"


/** Message sent to the code loader to ask for new code to be loaded.

		Arguments:
			dirName: directory where the new code is (previously generated)
			configs: configuration for existing waypoints
*/
<?php
grokit\create_message_type( 'LoadNewCodeMessage', [ 'dirName' => 'std::string', ], [ 'configs' => 'WayPointConfigurationList', ] );
?>


/** Message sent by the code loader when the new code was loaded.

		Arguments:
			code: new configuration objects for the waypoints
*/
<?php
grokit\create_message_type( 'LoadedCodeMessage', [ ], [ 'configs' => 'WayPointConfigurationList', ] );
?>


#endif // _CL_MESSAGES_H_
