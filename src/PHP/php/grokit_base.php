<?php
// Copyright 2013 Tera Insights, LLC. All Rights Reserved.

/*
 * This file contains functions and other definitions used by other php files
 * in DataPath.
 */

namespace grokit {
    // Returns $str with any characters in $chars doubled (this is how you
    // escape things in SQLite strings/identifiers)
    function doubleChars( $str, $chars ) {
        $strchars = str_split($str);
        $dblchars = str_split($chars);

        $ret = [];
        foreach( $strchars as $c ) {
            if( in_array($c, $dblchars) ) {
                $ret[] = $c;
            }

            $ret[] = $c;
        }

        return implode('', $ret);
    }

    // Generates a header for a generated file giving the copyright notice and
    // a notice that the file was generated from a PHP source.
    function generatedFileHeader( $dir_levels = 3 ) {
        $bt = debug_backtrace(DEBUG_BACKTRACE_IGNORE_ARGS);
        $bt_depth = \count($bt) - 1;
        $callingFile = $bt[$bt_depth]['file'];
        $callingFile = implode(DIRECTORY_SEPARATOR, array_slice(explode(DIRECTORY_SEPARATOR, $callingFile), -$dir_levels));

        $time = new \DateTime("now");
        $year = $time->format("Y");
        $isoDate = $time->format(\DateTime::ISO8601);

        return <<<EOT
/*
 * Copyright {$year} Tera Insights, LLC. All Rights Reserved.
 *
 * This file has been automatically generated.
 *
 * Source:          {$callingFile}
 * Generated on:    {$isoDate}
 *
 * Do not make modifications to this file, they will be wiped out on the next
 * compilation. Please make modifications to the source file instead.
 */
EOT
        ;
    }
}

namespace {

// Set default timezone
date_default_timezone_set('UTC');

// Separator used for comma separated lists
const CSL_SEP = ', ';

// Finds the first value in $keys that is a key in $arr and returns the value
// for that key.
function get_first_key( array $arr, array $keys,
                        $message = 'None of supplied keys found in array' ) {
    foreach( $keys as $key ) {
        if( array_key_exists($key, $arr) ) {
            return $arr[$key];
        }
    }

    grokit_error($message);
}

function get_first_key_default( array $arr, array $keys, $default ) {
    foreach( $keys as $key ) {
        if( array_key_exists($key, $arr) ) {
            return $arr[$key];
        }
    }

    return $default;
}

// Finds the first value in $values that appears in $arr and returns that
// value.
function get_first_value( array $arr, array $values ) {
    foreach( $values as $val ) {
        if( in_array($val, $arr) ) {
            return $val;
        }
    }

    grokit_error('None of supplied values found in array');
}

/**
 *  Function to get a value from an array if it exists and otherwise return a
 *  default.
 *
 *  If $key is a scalar value, $arr[$key] is returned if it exists, otherwise
 *  $default is returned.
 *
 *  If $key is an array, each value in $key is looked up into $arr in
 *  succession, and the value corresponding to the first matching key is
 *  returned. If there are no matches, $default is returned.
 */
function get_default( array $arr, $key, $default ) {
    if( is_scalar($key) ) {
        return array_key_exists($key, $arr) ? $arr[$key] : $default;
    }
    else if( is_array($key) ) {
        // Try keys in order given and return the first one we find.
        foreach( $key as $k ) {
            if( array_key_exists($k, $arr) )
                return $arr[$k];
        }

        // Didn't find any of the keys, return the default.
        return $default;
    }
    else {
        grokit_logic_error('Invalid key of type ' . gettype($key) . ' given to get_default');
    }
}

function array_get_key( array &$arr, $index ) {
    if( ! is_int($index) ) {
        $indexType = gettype($index);
        throw new InvalidArgumnetException("Array index must be integral, {$indexType} given");
    }

    if( $index < 0 || $index >= \count($arr) ) {
        $indMax = \count($arr);
        throw new OutOfBoundsException("Array index out of bounds. {$index} given, [0, {$indMax}) expected.");
    }

    return array_keys($arr)[$index];
}

function array_get_index( array &$arr, $index ) {
    return $arr[array_get_key($arr, $index)];
}

function array_update_index( array &$arr, $index, &$value ) {
    $arr[array_get_key($arr, $index)] = $value;
}

// Included for backwards compatibility
function array_set_index( array &$arr, $index, &$value ) {
    array_update_index($arr, $index, $value);
}

function hash_name( $str ) {
    // Hash string using md5 and return the upper 64 bits (16 hex characters)
    $hash = md5($str);
    return '0x' . substr($hash, 0, 16) . 'LL';
}

function args( $args ) {
    return implode(CSL_SEP, array_keys($args));
}

function typed_args( $args ) {
    $vals = [];

    foreach( $args as $n => $t ) {
        array_push($vals, "$t $n");
    }

    return implode(CSL_SEP, $vals);
}

function typed_ref_args( $args ) {
    $vals = [];

    foreach( $args as $n => $t ) {
        array_push($vals, "$t &$n");
    }

    return implode(CSL_SEP, $vals);
}

function const_typed_args( $args ) {
    $vals = [];
    foreach( $args as $n => $t ) {
        $vals[] = "const $t $n";
    }

    return implode(CSL_SEP, $vals);
}

function const_typed_ref_args( $args ) {
    $vals = [];

    foreach( $args as $n => $t ) {
        array_push($vals, "const $t &$n");
    }

    return implode(CSL_SEP, $vals);
}

function process_tuple_args( $inputs, $outputs ) {
  return const_typed_ref_args($inputs) . ", " . typed_ref_args($outputs);
}

function inputs_to_vector( array $args, $name = "item" ) {
    $vals = [];

    $counter = 0;
    foreach ($args as $n => $t ) {
        array_push($vals, "{$name}[{$counter}] = $n;");
        $counter++;
    }

    return implode(PHP_EOL, $vals) . PHP_EOL;
}

function declare_variables( array $vars, $const = false, $ref = false ) {
    $constText = $const ? 'const ' : '';
    $refText = $ref ? ' &' : '';
    $pattern = "{$constText}{val}{$refText} {key};" . PHP_EOL;

    return array_template($pattern, '', $vars);
}

/* Takes a pattern containing {key} constructs and replaces them with the value
 * mapped to key in the array $replace
 *
 * Example:
 *
 * template_replace('{fruit}s are delicious!', [ 'fruit' => 'Apple' ] )
 * returns 'Apples are delicious!'
 */
function template_replace( $pattern, array $replace ) {
    foreach( $replace as $key => $val ) {
        $pattern = str_replace( '{' . $key . '}', $val, $pattern );
    }

    return $pattern;
}

/*
 * For each KV pair in $array, performs a template_replace using $pattern and
 * with the key replacing {key} and {key1} and the value replacing {val} and
 * {val1}
 *
 * The results are then glued together using $glue
 */
function array_template( $pattern, $glue, array $array ) {
    $vals = [];
    foreach( $array as $name => $val ) {
        $replacers = [
            'key' => $name, 'key1' => $name,
            'val' => $val, 'val1' => $val,
            ];
        $vals[] = template_replace( $pattern, $replacers );
    }

    return implode( $glue, $vals );
}

/*
 * Performs the same as array_template, except a second array is iterated at
 * the same time, and its KV pairs replace {key2} and {val2}
 */
function array_template2( $pattern, $glue, array $array1, array $array2 ) {
    $vals = [];

    $c_1 = count($array1);
    $c_2 = count($array2);

    grokit_logic_assert($c_1 == $c_2,
        'Arrays passed to array_template2 do not have the same size');

    reset($array1);
    reset($array2);
    for( $i = 0; $i < $c_1; $i += 1 ) {
        $key1 = key($array1);
        $val1 = current($array1);
        $key2 = key($array2);
        $val2 = current($array2);

        $replacers = array(
            'key' => $key1, 'key1' => $key1,
            'val' => $val1, 'val1' => $val1,
            'key2' => $key2,
            'val2' => $val2,
        );

        $vals[] = template_replace( $pattern, $replacers );
    }

    return implode( $glue, $vals );
}

/*
 * template_implode( $pattern, $glue, $array1, ... )
 *
 * Generic version of array_template taking an arbitrary number of arrays. Each
 * KV in array N replaces {keyN} and {valN} with its key and value respectively.
 * Additionally, the KVs in the first array also replace {key} and {value} in
 * the pattern as well.
 */
function template_implode( $pattern, $glue, array $array1 ) {
    $n_args = func_num_args();

    $args = func_get_args();

    $arrays = array_splice($args, 2);

    $counts = array_map( 'count', $arrays );
    $count_freq = array_count_values($counts);
    grokit_logic_assert(count($count_freq) == 1,
        'Got arrays of different sizes in template_implode');

    $count = $counts[0];

    $replaced = [];

    foreach( $arrays as &$arr ) {
        reset($arr);
    }
    for( $i = 0; $i < $count; $i += 1 ) {
        $replacers = [];
        $j = 1;
        foreach( $arrays as &$arr ) {
            $keykey = 'key' . $j;
            $valkey = 'val' . $j;

            $replacers[$keykey] = key($arr);
            $replacers[$valkey] = current($arr);

            if( $j == 1 ) {
                $replacers['key'] = key($arr);
                $replacers['val'] = current($arr);
            }

            $j += 1;
            next($arr);
        }

        $replaced[] = template_replace( $pattern, $replacers );
    }

    return implode( $glue, $replaced );
}

/*
 *  Works much like template_implode, but the pattern is PHP code that is
 *  eval()'d.
 *
 *  Variables defined in the scope of the eval:
 *      $key{n}, $val{n}        The current key and value from array n
 *      $counter                The current index (numeric)
 */
function eval_implode($pattern, $glue)
{
    if (func_num_args() < 3)
        grokit_error("No arrays given");
    $numArrays = func_num_args() - 2;
    // Eval evaluates at global scope, can't use func_get_arg
    for ($counter = 1; $counter <= $numArrays; $counter++) {
        $dummyVar = func_get_arg($counter + 1);
        eval("\$array$counter = \$dummyVar;");
    }
    for ($counter = 1; $counter <= $numArrays; $counter ++)
        eval("if (!is_array(\$array$counter))
                  grokit_error(\"Non-array template args\");");
    $length = count($array1);
    for ($counter = 2; $counter <= $numArrays; $counter++) {
        eval("\$currentLength = count(\$array$counter);");
        if ($length != $currentLength)
            grokit_error("Unequal lengths of array arguments.");
    }
    $codeLines = array(); // array of c++ code
    for ($counter = 0; $counter < $length; $counter ++) {
        for ($setter= 1; $setter <= $numArrays; $setter++) {
            eval("\$key$setter = array_keys(\$array$setter)[$counter];");
            eval("\$val$setter = \$array{$setter}[\$key{$setter}];");
            eval("\$name$setter = ((is_numeric(\$key$setter)) ? 'V' : NULL) . \$key$setter;");
        }
        eval("\$codeLines[] = $pattern;");
    }
    return implode($glue, $codeLines);
}

// Generates a new name with the given prefix, unique within the current file.
function generate_name( $prefix = 'var_' ) {
    static $counter_map = [];

    if( !array_key_exists($prefix, $counter_map) ) {
        $counter_map[$prefix] = 0;
    }

    $newVal = $counter_map[$prefix];
    $counter_map[$prefix] += 1;
    return $prefix . $newVal;
}

function ensure_identifier( $val ) {
    $len = strlen($val);
    for( $i = 0; $i < $len; $i += 1 ) {
        if( $val[$i] != '_' && !ctype_alnum($val[$i]) ) {
            $val[$i] = '_';
        }
    }

    return $val;
}

/*
 *  Ensures the keys in the array $vars are valid variable names.
 *
 *  If a numeric key is found, the new name is the key prefixed with $prefix
 */
function ensure_valid_names( array $vars, $prefix = 'val' ) {
    $ret = [];
    foreach( $vars as $name => $val ) {
        if( is_numeric($name) ) {
            $name = $prefix . $name;
        }
        else {
            $name = ensure_identifier($name);
        }

        $ret[$name] = $val;
    }

    return $ret;
}

function ensure_unique_names( array $vars, $prefix = 'val' ) {
    $ret = [];
    foreach( $vars as $name => $val ) {
        if( is_numeric($name) ) {
            $name = generate_name($prefix);
        } else {
            $name = generate_name(ensure_identifier($name));
        }

        $ret[$name] = $val;
    }

    return $ret;
}

/*
 *  Performs a case-insensitive search for a string (needle) in an array
 *  (haystack).
 */
function in_array_icase( $needle, array $haystack ) {
    foreach( $haystack as $item ) {
        if( strcasecmp($needle, $item) == 0 )
            return true;
    }

    return false;
}

// Error detection and reporting

function grokit_fatal( $msg, Exception $reason = null ) {
    xdebug_print_function_stack($msg);
    throw new \RuntimeException($msg, $reason);
}

function grokit_error( $msg, Exception $reason = null ) {
    grokit_fatal( 'Fatal Runtime Error: ' . $msg, $reason );
}

function grokit_assert( $bool, $msg, Exception $reason = null ) {
    if( ! $bool ) {
        grokit_fatal( 'Runtime Assert Failed: ' . $msg, $reason );
    }
}

function grokit_error_if( $bool, $msg, Exception $reason = null ) {
    if( $bool ) {
        grokit_fatal('Fatal Runtime Error: ' . $msg );
    }
}

function grokit_logic_error( $msg, Exception $reason = null ) {
    grokit_fatal('Fatal Logic Error: ' . $msg, $reason);
}

function grokit_logic_error_if( $bool, $msg, Exception $reason = null ) {
    if( $bool ) {
        grokit_fatal('Fatal Logic Error: ' . $msg, $reason );
    }
}

function grokit_logic_assert( $bool, $msg, Exception $reason = null ) {
    if( ! $bool ) {
        grokit_fatal('Logic Assert Failed: ' . $msg, $reason );
    }
}

function grokit_warning( $msg ) {
    fwrite(STDERR, 'Warning: ' . $msg . PHP_EOL );
}

function grokit_warning_if( $bool, $msg ) {
    if( $bool ) {
        grokit_warning($msg);
    }
}

function grokit_warning_assert( $bool, $msg ) {
    if( ! $bool ) {
        grokit_warning($msg);
    }
}


} // end of global namespace

?>
