<?

// common functions

require_once('/opt/datapath/src/PHP/php/grokit_base.php');


/*
	Helper function to verify if a type is ok 
*/

function ensure_type($type){

	 if(is_string($type)){

		return typeInfo::getInfo($type);

	 } else if (is_array($type)){

		return typeInfo::getInfo($type);

	 }
	 return false;
}


/*
  helper function that registers aliases
  variables:
    ns: namespace
    name: name of type
    aliases: array with aliases
  
  todo: allow for namespace
*/
function declareAliasGlobal($ns, $name, $aliases){ 

  typeInfo::processInfoAliases($name, $aliases);
  return '';

}




/*
      Helper class to register types
      Will test if type has been registered before (checks aliases); if not, will register it

      Returns array with info (which is what the function returned)

      This class also keeps track of (system) headers; use getHeaders() and getSystemHeaders() to retrieve these
*/

class typeInfo {
 
 private static $types		= [];
 private static $aliases	= []; 
 private static $system_headers = [];
 private static $headers        = [];

 public function getInfo($type){


	 // is $type an alias?
	 $type = (array_key_exists($type, self::$aliases)) ? self::$aliases[$type] : $type;

	 // has it been defined before?
	 if (array_key_exists($type, self::$types)) return self::$types[$type]; 	  


	 // is there a function with that name?
	 if (function_exists($type)) return self::processInfo($type);

	 // no such function
	 throw new \RuntimeException("There is no function $type defined.");

 }

 // functions that get the arrays 

 public function getTypes() { return self::$types;}
 public function getAliases() { return self::$aliases;}
 public function getHeaders() { return self::$headers;}
 public function getSystemHeaders() { return self::$system_headers;}


 private function processInfo($type){

	 $info = $type();// call function with name $type

	 self::$types[$type] = $info;

	 self::processInfoHeaders($info);
	 self::processInfoSystemHeaders($info);
	 
	 return $info;
 }
 


 private function processInfoHeaders($info){

 	 if (!array_key_exists("headers", $info)) return;

	 foreach($info["headers"] as $h){
	 
		// skip if already exists
	 	if(in_array($h, self::$headers)) continue;

	 	self::$headers[] = $h;
	 }
 }

 private function processInfoSystemHeaders($info){

 	 if (!array_key_exists("system_headers", $info)) return;

	 foreach($info["system_headers"] as $h){
	 
		// skip if already exists
	 	if(in_array($h, self::$system_headers)) continue;

	 	self::$system_headers[] = $h;
	 }
 }


// called by declareAliasGlobal
 public function processInfoAliases($type, $aliasList){

	 foreach($aliasList as $alias){
	 
		// if alias already exist then throw error
	 	if(array_key_exists($alias, self::$aliases))   throw new \RuntimeException("Alias $alias already defined for {self::$aliases[$alias]}; cannot redeclare same alias for type $type.");

		// add alias to list of aliases
	 	self::$aliases[$alias]=$type; 
	 }

	 
  }
} //class


/*
      Helper class to register functions
      Will test if function name and arguments' types has been registered before; if not, will register it

      Returns array with info (which is what the function returned)

      todo: implement namespaces

*/

class functionInfo {
 
 private static $f	= []; // multiple dimension array [name][strTypes]

 public function getInfo($name, array $types = array() ){

 	// sort types
	$strTypes = implode("_",asort($types)); // for example "DOUBLE_INT_FLOAT"

	 // has it been defined before?
	 if (array_key_exists($name, self::$f)) {
	    if (array_key_exists($strTypes, self::$f[$name])) {
	    
	      return self::$f[$name][$strTypes];
	    }
	 }

	 // is there a function with that name?
	 if (function_exists($name)) return self::processInfo($name, $types, $strTypes);

	 // no such function
	 throw new \RuntimeException("There is no function $name defined.");

 }

 // function that get the array

 public function getFunctions() { return self::$f;}




 private function processInfo($name, $types, $strTypes){

	 $info = $name($types);// call function with name $type

	 self::$f[$name][$strTypes] = $info;
	 
	 return $info;
 }
 


} //class





?>
