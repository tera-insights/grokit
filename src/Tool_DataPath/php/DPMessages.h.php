/**
	This header file contains the messages used by DiskIO.
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



#ifndef _DP_MESSAGES_H_
#define _DP_MESSAGES_H_

#include <string>


////////// NEW PLAN //////////////////////

/** Message is sent by the main program to the coordinator with new xml files
        containing plans.

*/
<?php
grokit\create_message_type( 'NewPlan', [ 'confFile' => 'std::string', ], [ ] );
?>


#endif // _TRANS_MESSAGES_H_
