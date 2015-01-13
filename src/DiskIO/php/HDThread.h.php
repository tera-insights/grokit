#ifndef _HDTHREAD_H_
#define _HDTHREAD_H_

#include "EventProcessor.h"

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
require_once('HierarchiesFunctions.php');

grokit\interface_class( 'HDThread', 'EventProcessor', 'evProc' );
grokit\interface_constructor( [ 'fileName' => 'const char*', 'arrayHash' => 'uint64_t', 'dispatcher' => 'EventProcessor&', 'freqUpdate' => 'int', 'isReadOnly' => 'bool', ] );
grokit\interface_default_constructor();

grokit\interface_function('DiskNo', 'int', [ ]); 

// custom function 
?>
    
    static void CreateStripe(char* fileName, uint64_t arrayHash, int32_t stripeId, uint64_t offset){
	HDThreadImp::CreateStripe(fileName, arrayHash, stripeId, offset);
    }

<?php
grokit\interface_class_end();
?>


#endif // _HDTHREAD_H_
