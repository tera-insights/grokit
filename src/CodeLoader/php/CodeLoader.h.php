#ifndef _CODELOADER_H_
#define _CODELOADER_H_

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
?>


<?php
grokit\interface_class( 'CodeLoader', 'EventProcessor', 'evProc' );
?>

	<?php
grokit\interface_constructor( [ 'srcDir' => 'const char *', 'coordinator' => 'EventProcessor&', ] );
?>

	<?php
grokit\interface_default_constructor();
?>

<?php
grokit\interface_class_end();
?>



#endif // _CODELOADER_H_
