<?
//
//  Copyright 2012 Alin Dobra and Christopher Jermaine, Apache 2.0
//  Copyright 2013, Tera Insights. All rights reserved

function STRING_LITERAL(){ ?>

/**
   This defines a type for string literals, which are considered by be of type
   const char *. This allows for literal strings to be handled differently than
   string variables, which is required as datapath does not use const char *
   as its string type, and thus all string literals must be converted to
   another type in order to be useful.
 */


typedef const char* STRING_LITERAL;

<?

return array(
    'kind' => 'TYPE',
    "complex" => false
);

} // end of function

?>

