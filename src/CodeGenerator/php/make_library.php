<?
// copyright 2013 Tera Insights LLC  




// note: a better test procedure is in basePHP/Test.php
// ==================================================== 


// This script creates a php library of all the types, functions, GLAs etc in the base library


require_once('/opt/datapath/src/PHP/php/grokit_base.php');
require_once 'make_library_helper_functions.php';

// Include all .php files in all subdirectories (for now: manually)

// start output buffering
ob_start();

// include types 
include '/opt/datapath/base/Libs/base-php/Types/DOUBLE.h.php';
include '/opt/datapath/base/Libs/base-php/Types/FLOAT.h.php';


include '/opt/datapath/base/Libs/base-php/GLAs/TopKGLA.h.php';

$t=gla_topK(array("DOUBLE","DOUBLE", "FLOAT", "REAL"),"topKGLA");

// capture buffer
$buffer = ob_get_contents();


// clear buffer
ob_end_clean();
 
print_r(typeInfo::getSystemHeaders());
print_r(typeInfo::getAliases()); 
print_r(typeInfo::getTypes());

?>