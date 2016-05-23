<?
function CSVReader( array $t_args, array $output ) {
    $debug = get_default($t_args, 'debug', 0);

    $simple = get_default($t_args, 'simple', false);
    $trimCR = get_default($t_args, 'trim.cr', false);
    $lineNumber = get_default($t_args, 'line.number', false);

    if ($lineNumber) {
        $lineColumn = array_keys($output)[0];
        $my_output = array_slice($output, 1);
    } else {
        $my_output = $output;
    }

    // Handle separator
    $separator = ',';
    if( array_key_exists( 'sep', $t_args) || array_key_exists('separator', $t_args) ) {
        $sep = get_first_key( $t_args, [ 'sep', 'separator' ] );

        grokit_assert( is_string($sep),
            "Got " . gettype($sep) . " instead of string for separator.");

        if( strtolower( $sep ) === 'tab' ) $sep = '\t';

        grokit_assert( $sep != "\n",
            'CSV column delimiter cannot be new line');

        // Scream if separator is longer than one character
        grokit_assert( \strlen($sep) == 1 || $sep == '\t',
            'Expected string of length 1 for separator, got string <' . $sep . '> instead');

        $separator = $sep;
    }

    // Handle quote character
    $quotechar = '"';
    if( array_key_exists( 'quote', $t_args) && !is_null($t_args['quote']) ) {
        grokit_assert(!$simple, 'Quote option not available for simple CSVReader');
        $quote = $t_args['quote'];

        grokit_assert( is_string($quote),
            "Got " . gettype($quote) . " instead of string for quote.");

        // Scream if separator is longer than one character
        grokit_assert( \strlen($quote) == 1,
            'Expected string of length 1 for quote character, got string <' . $quote . '> instead');

        $quotechar = $quote;
    }
    $quotechar = addcslashes($quotechar, '\\\'');

    // Handle escape character
    $escapeChar = '\\';
    if( array_key_exists( 'escape', $t_args ) && !is_null($t_args['escape']) ) {
        grokit_assert(!$simple, 'Escape option not available for simple CSVReader');
        $escape = $t_args['escape'];

        grokit_assert( is_string($escape),
            'Got ' . gettype($escape) . ' instead of string for escape character.');

        grokit_assert( \strlen($escape) == 1,
            'Expected string of length 1 for escape character, got string <' . $escape . '> instead');

        $escapeChar = $escape;
    }
    $escapeChar = addcslashes($escapeChar, '\\\'');

    // Handle header lines
    $headerLines = 0;
    if( array_key_exists( 'skip', $t_args) ) {
        $headerLines = $t_args['skip'];

        grokit_assert( is_int($headerLines),
            'Got ' . gettype($headerLines) . ' instead of int for number of lines to skip.');

        grokit_assert( $headerLines >= 0,
            'Cannot skip a negative number of lines.');
    }

    // Maximum number of lines to read
    $maxLines = get_default($t_args, 'n', -1);
    grokit_assert( is_int($maxLines), 'Got ' . gettype($maxLines) . ' instead of int for template argument "n"');

    $nullArg = get_first_key_default($t_args, ['nullable'], false);

    $nullable = [];
    $nullStr = [];
    foreach( $my_output as $name => $type ) {
        $nullable[$name] = false;
    }

    if( $nullArg === true ) {
        foreach( $my_output as $name => $type ) {
            $nullable[$name] = true;
            $nullStr[$name] = 'NULL';
        }
    } else if( is_array($nullArg) ) {
        foreach( $nullArg as $n => $v ) {
            // If nullable value is an associative mapping, the value is either true/false
            // or the value of the null string
            if( is_string($n) ) {
                grokit_assert(is_string($v) || is_bool($v),
                    'CSVReader: nullable associative mapping must have string or boolean values');
                grokit_assert(array_key_exists($n, $nullable), 'CSVReader: cannot make unknown attribute ' . $n . ' nullable');

                if( is_bool($v) ) {
                    $nullable[$n] = $v;
                    $nullStr[$n] = 'NULL';
                } else {
                    $nullable[$n] = true;
                    $nullStr[$n] = $v;
                }
            } else {
                if( is_array($v) ) {
                    grokit_assert(array_key_exists('attr', $v), 'CSVReader: Name of nullable attribute not specified');
                    $attrName = $v['attr']->name();

                    $nullable[$attrName] = true;
                    $nullStr[$attrName] = array_key_exists('null', $v) ? $v['null'] : 'NULL';
                } else {
                    // Otherwise, it's just nullable
                    $attrName = $v->name();
                    grokit_assert(array_key_exists($attrName, $nullable), 'CSVReader: cannot make unknown attribute ' . $v . ' nullable');
                    $nullable[$attrName] = true;
                    $nullStr[$attrName] = 'NULL';
                }
            }
        }
    } else if( $nullArg === false ) {
        // Nothing
    } else if( is_string($nullArg) ) {
        foreach( $my_output as $name => $type ) {
            $nullable[$name] = true;
            $nullStr[$name] = $nullArg;
        }
    } else {
        grokit_error('Template argument "nullable" must be boolean or array, ' . typeof($nullArg) . ' given' );
    }

    // Come up with a name for ourselves
    $className = generate_name( 'CSVReader' );

    if( $debug >= 2 ) {
        foreach( $my_output as $name => $type ) {
            fwrite(STDERR, "CSVReader: {$name} is nullable: " . ($nullable[$name] ? 'true' : 'false') . PHP_EOL);
        }
    }
?>

class <?=$className?> {
    std::istream& my_stream;
    std::string fileName;

    // Template parameters
    static constexpr size_t MAX_LINES = <?=$maxLines?>;
    static constexpr size_t HEADER_LINES = <?=$headerLines?>;
    static constexpr char DELIMITER = '<?=$separator?>';
<?  if( !$simple ) { ?>
    static constexpr char QUOTE_CHAR = '<?=$quotechar?>';
    static constexpr char ESCAPE_CHAR = '<?=$escapeChar?>';

    typedef boost::escaped_list_separator<char> separator;
    typedef boost::tokenizer< separator > Tokenizer;
    separator my_separator;
    Tokenizer my_tokenizer;
<?  } ?>

    // Prevent having to allocate this every time.
    std::string line;
    std::vector<std::string> tokens;

    size_t count;

<?  \grokit\declareDictionaries($my_output); ?>

public:

    <?=$className?> ( GIStreamProxy& _stream ) :
        my_stream(_stream.get_stream())
        , fileName(_stream.get_file_name())
<?  if( !$simple ) { ?>
        , my_separator(ESCAPE_CHAR, DELIMITER, QUOTE_CHAR)
        , my_tokenizer(std::string(""))
<?  } ?>
        , count(0)
    {
<?  if( $headerLines > 0 ) { ?>
        for( size_t i = 0; i < HEADER_LINES; ++i ) {
            FATALIF( !getline( my_stream, line ), "CSV Reader reached end of file before finishing header.\n" );
        }
<?  } // If headerLines > 0 ?>
    }

// >

    bool ProduceTuple( <?=typed_ref_args($output)?> ) {
        if (count < MAX_LINES) { //>
            count++;
<?  if ($lineNumber) { ?>
            <?=$lineColumn?> = count;
<?  } ?>
        } else {
            return false;
        }

        if( getline( my_stream, line ) ) {
<?  if( $trimCR ) { ?>
            if( line.back() == '\r' ) {
                line.pop_back();
            }
<? } // if trimCR ?>
<?  if( !$simple ) { ?>
<?      if( $debug >= 1 ) { ?>
            try {
<?      } // if debug >= 1 ?>
            my_tokenizer.assign( line, my_separator );
<?      if( $debug >= 1 ) { ?>
            } catch(...) {
                FATAL("CSVReader for file %s failed on line: %s", fileName.c_str(), line.c_str());
            }
<?      } // if debug >= 1 ?>
            Tokenizer::iterator it = my_tokenizer.begin();

<?
        foreach( $my_output as $name => $type ) {
            if( $nullable[$name] ) { // nullable
?>
            <?\grokit\fromStringNullable($name, $type, 'it->c_str()', true, $nullStr[$name]);?>

<?          } else { // not nullable ?>
            <?=\grokit\fromStringDict($name, $type, 'it->c_str()')?>;
<?
            } // end nullable check
?>
            ++it;
<?
        } // foreach output
?>
<?  } // if complex reader
    else {
?>
            for( char & c : line ) {
                if( c == DELIMITER )
                    c = '\0';
            }

            const char * ptr = line.c_str();
<?
        $first = true;
        foreach( $my_output as $name => $type ) {
            if( $first ) {
                $first = false;
            }
            else {
?>
            while( *(ptr++) != '\0' )
                ; // Advance past next delimiter
<?          } // not first output ?>
<?          if( $nullable[$name] ) { ?>
            <?=\grokit\fromStringNullable($name, $type, 'ptr', true, $nullStr[$name]); ?>
<?          } else { // not nullable ?>
            <?=\grokit\fromStringDict($name, $type, 'ptr')?>;
<?          } // if nullable ?>
<?      } // foreach output ?>
<?  } // if simple reader ?>

            return true;
        }
        else {
            return false;
        }
    }

<?  \grokit\declareDictionaryGetters($my_output); ?>
};

<?
    $sys_headers = [ 'vector', 'string', 'iostream', 'cstdint' ];
    if( !$simple )
        $sys_headers[] = 'boost/tokenizer.hpp';

    return [
        'name' => $className,
        'kind' => 'GI',
        'output' => $output,
        'system_headers' => $sys_headers,
        'user_headers' => [
            'GIStreamInfo.h',
            'Dictionary.h',
            'DictionaryManager.h'
        ]
    ];
}

?>
