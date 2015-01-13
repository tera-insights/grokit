<?php
// Copyright 2013 Tera Insights, LLC. All Rights Reserved.

/*
 * This file contains useful functions used during code generation.
 */

namespace grokit\internal {
    // Gets the name of the metadata database.
    function getMetadataDB() {
        return $_SERVER['GROKIT_EXEC_PATH'] . DIRECTORY_SEPARATOR . 'datapath.sqlite';
    }

    function my_is_int($val) {
        if( is_int($val) ) return true;

        if( is_string($val) ) {
            if( $val[0] == '-' ) {
                return ctype_digit(substr($val, 1));
            } else {
                return ctype_digit($val);
            }
        }

        return false;
    }

    function valueToJsonCpp($var, $val, $initialize = true) {
        if( is_numeric($val) ) {
?>
    <?=$var?> = <?=$val?>;
<?
        } else if( is_string($val) ) {
?>
    <?=$var?> = "<?=addcslashes($val, "\0..\037\177..\377\\\"")?>";
<?
        } else if( is_null($val) ) {
?>
    <?=$var?> = Json::Value(Json::nullValue);
<?
        } else if( is_array($val) || is_object($val) ) {
            // Determine if all of the keys are integers are not.
            $allInt = true;
            foreach( $val as $name => $value ) {
                $allInt = $allInt && my_is_int($name) && (intval($name) >= 0);
            }

            if( $initialize ) {
                if( $allInt ) {
?>
    <?=$var?> = Json::Value(Json::arrayValue);
<?
                } else {
?>
    <?=$var?> = Json::Value(Json::objectValue);
<?
                }
            }

            foreach( $val as $name => $value ) {
                $key = $allInt ? intval($name) : '"' . $name . '"';
                $key = $key === 0 ? '0u' : $key; // compiler has issues if key is an array index at 0
                valueToJsonCpp($var . "[{$key}]", $value, true);
            }
        } else {
            throw new Exception('Can not serialize value of type ' . gettype($val) . ' to JSON');
        }
    }
}

namespace {
    // Function to help serialize
    function SerializeJson( $dest, $vals, $type = null ) {
?>
    <?=$dest?> = Json::Value(Json::objectValue);
<?
        if( !is_null($type) ) {
?>
    <?=$dest?>["__type__"] = <?=$type?>;
<?
        } // if type is not null

        foreach( $vals as $name => $var ) {
?>
    ToJson(<?=$var?>, <?=$dest?>["<?=$name?>"]);
<?
        } // for each value to serialize
    }

    // Function to help serialize to the JSON wrapper type.
    // $arg can either be a string (in which case it is the name of an already
    // constructed Json::Value which will be packed into dest), or an array
    // of variable => name, which will be used to serialize the variables
    // given. The optional type will be passed along to SerializeJson if given
    // and $arg is an array.
    function ProduceResult( $dest, $arg, $type = null ) {
        ob_start();
        if( is_string($arg) ) {
?>
    <?=$dest?>.set(<?=$arg?>);
<?
        } // if arg is string
        else if( is_array($arg) ) {
?>
    {
        Json::Value tmp;
        <?SerializeJson('tmp', $arg, $type);?>
        <?=$dest?>.set(tmp);
    }
<?
        } // if arg is array
        else {
            grokit_error('ProduceResult: second argument is not a string or an array!');
        }

        $res = ob_get_clean();
        return $res;
    }

    /**
     * Generates functions used to generate code to describe a type as JSON.
     *
     * @param string $type a string describing the type (e.g. 'integer', 'string')
     * @param callable $extra a callable that takes a single parameter, which
     * is the name of the Json::Value object in which to store additional
     * information. If no callable given, no additional information will be
     * added.
     *
     * @return callable A callable that is suitable for the 'describe_json'
     * return value of a type generation function.
     */
    function DescribeJson($type, callable $extra = null) {
        return function($var, $dType) use ($type, $extra) {
?>
    {
        <?=$var?>["__type__"] = "<?=$type?>";
<?
            if( is_callable($extra) ) {
                $extra($var, $dType);
            }
?>
    }
<?
        };
    } // DescribeJson

    /**
     * Generates functions adding static data describing a type. Intended to be
     * used to create a function to pass as the second argument to
     * DescribeJson().
     *
     * @param mixed $info an array or simple object containing key-value
     * pairs of information to include.
     *
     * @return callable a callable that is suitable to be passed as the second
     * parameter to DescribeJson().
     */
    function DescribeJsonStatic($info) {
        return function($var) use ($info) {
            grokit\internal\valueToJsonCpp($var, $info, false);
        };
    } // DescribeJsonStatic
} // global namespace

namespace grokit {

    // Declares dictionaries for a list of name => type pairs, if any of the
    // types require dictionaries.
    function declareDictionaries( array &$vars ) {
        // Array of declared dictionaries so we don't declare them twice.
        $declared_dicts = [];

        foreach( $vars as $name => $type ) {
            if( ! is_object($type) ) {
                fwrite(STDERR, "declarDictionaries: Type for value " . $name . " is not an object." . PHP_EOL);
                fwrite(STDERR, "declarDictionaries: vars: " . print_r($vars, true) . PHP_EOL);
                grokit_error("Invalid type given to declareDictionaries for var " . $name);
            }

            if( $type->reqDictionary() ) {
                $dict = $type->dictionary();

                if(! in_array($dict, $declared_dicts)) {
                    $declared_dicts[] = $dict;
?>
    Dictionary <?=$dict?>_local_dictionary;
<?
                }
            }
        }
    }

    // Declares functions used by the GI waypoint to get any local dictionaries used by the
    // GI.
    function declareDictionaryGetters( array &$vars ) {
        // Array of declared dictionaries so we don't declare them twice.
        $declared_dicts = [];

        foreach( $vars as $name => $type ) {
            if( $type->reqDictionary() ) {
                $dict = $type->dictionary();

                if(! in_array($dict, $declared_dicts)) {
                    $declared_dicts[] = $dict;
?>
    Dictionary& get_dictionary_<?=$dict?> ( void ) {
        return <?=$dict?>_local_dictionary;
    }
<?
                }
            }
        }
    }

    // Constructs $name from $str, using the dictionary defined by declare_dictionaries()
    // if needed.
    function fromStringDict( $name, $type, $str ) {
        if( $type->reqDictionary() ) {
            $dict = $type->dictionary() . '_local_dictionary';
            return 'FromString(' . $name . ', ' . $str . ', ' . $dict . ' )';
        }
        else {
            return 'FromString(' . $name . ', ' . $str . ')';
        }
    }

    // Determines the number of elements in a dictionary.
    // This can be used to determine the current cardinality of a factor at
    // code generation time.
    function dictionarySize( $name ) {
        $dbFile = internal\getMetadataDB();
        $name = doubleChars($name, '"');

        $db = new \SQLite3($dbFile);

        $dname = "Dictionary_$name";

        // Ensure the table exists
        $db->exec("CREATE TABLE IF NOT EXISTS \"$dname\"( \"id\" INTEGER, \"order\" INTEGER, \"str\" TEXT );");

        $ret = $db->querySingle("SELECT COUNT(*) FROM \"Dictionary_$name\";");

        if( !$ret ) {
            return 0;
        }
        else {
            return $ret;
        }

        $db->close();
    }

    function getNull( $type ) {
        $typename = $type->value();
        if( $type->is('_primative_') ) {
            return "{$typename}_Null( GrokitNull::Value )";
        } else {
            return "{$typename}( GrokitNull::Value )";
        }
    }

    function fromStringNullable( $name, $type, $str, $dict = true, $nullStr = 'NULL', $case = true ) {
        $nullChars = \strlen($nullStr) > 0 ? str_split($nullStr) : [];
        $nullChars[] = "\\0";
        $i = 0;

        $nullStr = $case ? $nullStr : strtoupper($nullStr);

        $nullCheck = "true";
        foreach( $nullChars as $c ) {
            $nullCheck .= " && ${str}" . '[' . $i++ . '] == \'' . $c . '\'';
        }
?>
    {
        if( <?=$nullCheck?> ) {
            <?=$name?> = <?=getNull($type)?>;
        } else {
            <?=$dict ? fromStringDict($name, $type, $str) : "FromString({$name}, {$type})"?>;
        }
    }
<?
    }

} // namespace grokit

?>
