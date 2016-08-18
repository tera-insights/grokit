<?php

// Copyright 2013 Tera Insights, LLC. All Rights Reserved.

/*
 * This file contains the definitions for the Library Manager, which handles
 * the data types, functions, operators, and other objects defined by libraries.
 *
 * The Library Manager handles automatic conversions between types.
 */

namespace grokit {

    // Represents an entry in the global alias table.
    // Contains both the information and code generated for a type.
    class GlobalAlias {
        private $info;
        private $code;

        public function __construct( $info, $code ) {
            $this->info = $info;
            $this->code = $code;
        }

        public function info() { return $this->info; }
        public function code() { return $this->code; }
    }

    // Represents a concrete function registered with the system
    class RegisteredFunction {
        // The name of the function
        private $name;

        // The arguments to the function (list of type names)
        private $args;

        // callable to call when instantiating operator
        private $call;

        // Create a new registered function with a given name, set of arguments, and
        // a callable used to instantiate the function when needed.
        public function __construct( $name, array $args, callable $call ) {
            $this->name = $name;
            $this->args = array_map('strtoupper', $args);
            $this->call = $call;
        }

        public function name() { return $this->name; }
        public function args() { return $this->args; }
        public function call() { return $this->call; }

        /**
         *  Returns an integer describing the compatibility of the function
         *  with the given types.
         *
         *  @returns -1 if incompatible
         *      0 if perfectly compatible
         *      otherwise the number of implicit conversions required to make compatible
         */
        public function compatibility( array $types, $exact = false ) {
            if( count($types) != count($this->args) ) {
                return -1;
            }

            $types = array_map("strtoupper", $types);

            $rating = 0;
            foreach( $this->args as $name => $expected ) {
                $given = $types[$name];

                if( $expected != $given ) {
                    if( $exact ) return -1;
                    if( canConvert( $given, $expected ) ) {
                        $rating += 1;
                    }
                    else {
                        return -1;
                    }
                }
            }

            return $rating;
        }

        public function __toString() {
            return $this->name . '(' . implode(', ', $this->args) . ')';
        }

    }

    class LibraryManager {

        /***** Class Members *****/
        // Mapping from alias to base name
        private $aliases = [];

        //Mapping from hash value to type info
        private $typeCache = [];

        // Mapping of name to lists of functions and operators registered with
        // that name.
        private $registeredFunctions = [];

        // Mapping from a hash to a function information object for functions
        // and operators that have been instantiated.
        private $functionCache = [];

        // Buffer of generated code.
        private $buffer = "";

        // Set of headers to include.
        private $headers = [];

        // Set of library headers included
        // Mapping from header name to code
        private $libHeaders = [];

        public function __construct() {
        }

        /***** Aliasing *****/

        public function normalizeName( $name ) {
            // If  the name of the type isn't namespaced and not otherwise
            // aliased, assume the default namespace.

            if( ! self::IsNamespaced($name) ) {
                $name = self::JoinNamespace('base', $name);
            }

            $name = \strtoupper($name);

            if(array_key_exists( $name, $this->aliases )) {
                $name = $this->aliases[$name];
            }

            return $name;
        }

        public function normalizeNameFunc( $name ) {
            if( ! self::IsNamespaced($name) ) {
                $name = self::JoinNamespace('base', $name);
            }

            return \strtoupper($name);
        }

        public function addAlias( $alias, $base ) {
            $alias = \strtoupper($alias);
            $base = \strtoupper($base);
            $this->aliases[$alias] = $base;
        }

        /***** Header Management *****/

        public function addHeader( $header ) {
            if( !in_array($header, $this->headers) ) {
                $this->headers[] = $header;
            }
        }

        public function addLibHeader( $header ) {
            // Strip any file extension
            $header_parts = explode('.', $header);
            $nParts = \count($header_parts);
            if( $nParts > 1 ) {
                $header_parts = array_slice($header_parts, 0, $nParts - 1);
                $header = implode('.', $header_parts);
            }

            if( !in_array($header, $this->libHeaders) ) {
                $buffer = "";
                $args = [];

                $name_parts = explode('\\', $header);
                $name = implode('::', $name_parts);

                // All include PHP functions are in the <lib>\includes namespace
                $ns_parts = array_slice($name_parts, 0, -1);
                $ns_parts[] = 'includes';
                $func_parts = array_merge($ns_parts, array_slice($name_parts, -1, 1));
                $func = implode('\\', $func_parts);

                $this->CallGenerator($name, $args, $func, $buffer);

                $this->libHeaders[$header] = $buffer;
            }
        }

        /***** Type Management *****/

        private function defineType( $name, $info ) {
            grokit_logic_assert( ! array_key_exists($name, $this->typeCache),
                'Attempted to multiply define type ' . $name );

            $this->typeCache[$name] = $info;
        }

        // Look up to see if a type has already been instantiated and cached.
        public function typeDefined( $name ) {
            if( array_key_exists($name, $this->typeCache) ) {
                //fwrite(STDERR, $name . ' defined already' . PHP_EOL);
                return true;
            }
            else {
                //fwrite(STDERR, $name . ' not defined already' . PHP_EOL);
                return false;
            }
        }

        public function getDefinedType($hash) {
            if( array_key_exists($hash, $this->typeCache) ) {
                return $this->typeCache[$hash];
            }
            else {
                throw new Exception("No type with hash {$hash} defined");
            }
        }

        public function lookupType( $name, array &$args = [], $ohash = null) {
            $oName = $name;
            $name = $this->normalizeName($name);
            $declared = 'false';

            if( self::HasTypeDeclaration($name) ) {
                $decl = self::GetTypeDeclaration($name, $args[0]);
                $name = $this->normalizeName($decl['name']);
                $args[0] = $decl['targs'];
                $declared = 'true';
            }

            // Cook up hash of the type to perform lookup
            $hasher = hash_init( 'sha256' );
            hash_update( $hasher, $name );

            // args is an array of arrays, so hash each value.
            foreach( $args as $n => &$arg_set ) {
                if( count($arg_set) > 0 ) {
                    hash_update( $hasher, $n );
                    if( $n > 0 )
                        hash_update( $hasher, hashArgList( array_values($arg_set) ) );
                    else
                        hash_update( $hasher, hashArgList( $arg_set ) );
                }
            }

            $hash = hash_final($hasher);
            //fwrite( STDERR, "LOOKUP: name({$name}) oName({$oName}) declared({$declared}) args(" . squashToString($args) . ") hash({$hash})" . PHP_EOL);

            // If it is in the cache, look it up there first.
            if( $this->typeDefined( $hash ) ) {
                return $this->typeCache[$hash];
            }

            if( !is_null($ohash) && $this->typeDefined($ohash) ) {
                return $this->typeCache[$ohash];
            }

            $hash = is_null($ohash) ? $hash : $ohash;
            $args[] = $hash;

            $buffer = "";
            $res = self::CallGeneratorType( $name, $args, $buffer );

            if (is_typeinfo($res)) {
                return $res;
            }

            grokit_logic_assert(count($this->buffer) > 0,
                "Called generator for a type but the buffer is empty!");

            // If the result specified a type, use that instead, and remember
            // the original hash so that we can lookup this type by either.
            $hash_orig = $hash;
            if( array_key_exists( 'hash', $res ) ) {
                $hash = $res['hash'];
            }

            // Extract the kind from the result of the generator and create
            // the type info based on what was returned.
            $kind = $res['kind'];
            $tName = $res['name'];
            $tInfo = null;
            switch( $kind ) {
            case InfoKind::T_TYPE:
                $tInfo = new DataType( $hash, $name, $tName, $res, $args );
                break;
            case InfoKind::T_GI:
                // args = [targs, outputs, states]
                if (\count($args) > 3 && !array_key_exists('required_states', $res)) {
                    $res['required_states'] = $args[2];
                }
                $tInfo = new GI_Info( $hash, $name, $tName, $res, $args );
                break;
            case InfoKind::T_GLA:
                // args = [targs, inputs, outputs, states]
                if (\count($args) > 4 && !array_key_exists('required_states', $res)) {
                    $res['required_states'] = $args[3];
                }
                $tInfo = new GLA_Info( $hash, $name, $tName, $res, $args );
                break;
            case InfoKind::T_GF:
                // args = [targs, inputs, states]
                if (\count($args) > 3 && !array_key_exists('required_states', $res)) {
                    $res['required_states'] = $args[2];
                }
                $tInfo = new GF_Info( $hash, $name, $tName, $res, $args );
                break;
            case InfoKind::T_GIST:
                // args = [targs, outputs, states]
                if (\count($args) > 3 && !array_key_exists('required_states', $res)) {
                    $res['required_states'] = $args[2];
                }
                $tInfo = new GIST_Info( $hash, $name, $tName, $res, $args );
                break;
            case InfoKind::T_GT:
                // args = [targs, inputs, otuputs, states]
                if (\count($args) > 4 && !array_key_exists('required_states', $res)) {
                    $res['required_states'] = $args[3];
                }
                $tInfo = new GT_Info( $hash, $name, $tName, $res, $args );
                break;
            case InfoKind::T_GSE:
                $tInfo = new GSE_Info( $hash, $name, $tName, $res, $args );
                break;
            case InfoKind::T_RESOURCE:
                $tInfo = new Resource_Info($hash, $name, $tName, $res, $args);
                break;
            default:
                grokit_error( 'Unknown kind ' . $kind . ' given by return value from generator for ' .
                    $name );
            }

            // Cache the information
            // Only do so if it is not already defined, as the function could
            // have returned the information for an already defined type.
            //
            // Cache the information by both the original and returned hash
            // (if different)
            if( ! $this->typeDefined( $hash ) ) {
                $this->defineType( $hash, $tInfo );
            }

            if( ! $this->typeDefined( $hash_orig ) ) {
                $this->defineType( $hash_orig, $tInfo );
            }

            // Also define the type by the hash of the name returned.
            $nHash = hash( 'sha256', \strtoupper($tName) );
            if( ! $this->typeDefined( $nHash ) ) {
                $this->defineType( $nHash, $tInfo );
            }

            // Create opening tag and add to the buffer
            $oTag = [ 'kind' => $kind, 'name' => $tName, 'action' => 'start', 'summary' => $tInfo->summary() ];
            $this->buffer .= '//+' . json_encode($oTag) . PHP_EOL;

            // Add the local buffer to the current buffer
            $this->buffer .= $buffer;

            // Create closing tag and add to the buffer
            $cTag = [ 'kind' => $kind, 'name' => $tName, 'action' => 'end' ];
            $this->buffer .= '//+' . json_encode($cTag) . PHP_EOL;

            return $tInfo;
        }

        /***** Function management *****/

        public function isFunctionDefined( $hash ) {
            return array_key_exists( $hash, $this->functionCache );
        }

        public function getDefinedFunction( $hash ) {
            grokit_assert( $this->isFunctionDefined($hash),
                'Attempting to get cached function information for undefined function.');

            return $this->functionCache[$hash];
        }

        // Looks and sees whether or not a function with this exact signature has been registered.
        public function functionRegistered( $name, array $args, $exact = false ) {
            $name = strval($name);
            $name = \strtoupper($name);
            if( ! array_key_exists( $name, $this->registeredFunctions ) ) {
                return false;
            }

            $args = array_map("strtoupper", $args);

            $possible = & $this->registeredFunctions[$name];

            foreach( $possible as $candidate ) {
                if( $candidate->compatibility($args, $exact) >= 0 ) {
                    return true;
                }
            }

            return false;
        }

        public function registerFunction( $name, array $args, callable $call ) {
            $oName = $name;
            $name = \strtoupper($name);
            $args = array_map("strtoupper", $args);
            grokit_assert( ! $this->functionRegistered($name, $args, true),
                'Attempting to register duplicate function ' . $name .
                ' with arguments (' . implode(', ', $args) . ')' );

            $my_info = new RegisteredFunction( $oName, $args, $call );

            if( !array_key_exists( $name, $this->registeredFunctions ) ) {
                $this->registeredFunctions[$name] = [];
            }

            $this->registeredFunctions[$name][] = $my_info;
        }

        private function generateFunction( $name, array &$args, array &$targs, $hash, $call = null ) {
            $name_parts = self::SplitNamespace($name);

            if( $call === null ) {
                $call = self::PHP_NS_SEP . implode(self::PHP_NS_SEP, $name_parts);
            }

            $allArgs = [ $args, $targs ];

            $buffer = "";
            $res = self::CallGenerator( $name, $allArgs, $call, $buffer );
            grokit_logic_assert(count($this->buffer) > 0,
                "Called generator for a function but the buffer is empty!");

            // Build the Function/Operator info
            $tName = $res['name'];
            $kind = $res['kind'];

            if( array_key_exists( 'hash', $res ) ) {
                $hash = $res['hash'];
            }

            switch( $kind ) {
            case InfoKind::T_FUNC:
                $info = new FunctionInfo( $hash, $name, $tName, $res, $targs );
                break;
            case InfoKind::T_OP:
                $info = new OperatorInfo( $hash, $name, $tName, $res );
                break;
            default:
                grokit_error( 'Looked up function/operator ' . $name . ', got a '. $kind .
                    'instead.');
            }

            // Create opening tag and add to the buffer
            $oTag = [ 'kind' => $kind, 'name' => $tName, 'action' => 'start' , 'summary' => $info->summary() ];
            $this->buffer .= '//+' . json_encode($oTag) . PHP_EOL;

            // Add the local buffer to the current buffer
            $this->buffer .= $buffer;

            // Create closing tag and add to the buffer
            $cTag = [ 'kind' => $kind, 'name' => $tName, 'action' => 'end' ];
            $this->buffer .= '//+' . json_encode($cTag) . PHP_EOL;

            return $info;
        }

        public function lookupConcreteFunction( $name, array &$args, $hash ) {
            $oName = $name;
            $name = \strtoupper($name);
            grokit_assert( array_key_exists( $name, $this->registeredFunctions ),
                'No functions registered with the name ' . $name );

            $candidates = & $this->registeredFunctions[$name];

            // Matches is a mapping from the score to the candidate RegisteredFunction
            // We keep track of all possible matches in case we have multiple possibilities,
            // and we may in the future be able to print nice error messages.
            $matches = [];
            $nMatches = 0;

            $fStr = $name . '('. implode(', ', $args) . ')';
            //fwrite(STDERR, 'Looking up ' . $fStr . PHP_EOL);
            foreach( $candidates as $cand ) {
                $rating = $cand->compatibility( $args );

                //fwrite(STDERR, 'Candidate: ' . $cand . ' Rating: ' . $rating . PHP_EOL );

                if( $rating >= 0 ) {
                    if( ! array_key_exists( $rating, $matches ) ) {
                        $matches[$rating] = [];
                    }

                    $matches[$rating][] = $cand;
                    $nMatches += 1;
                }
            }

            grokit_logic_assert( ! array_key_exists( 0, $matches ) || count($matches[0]) == 1,
                'Got more than one exact match for function ' . $fStr );

            //fwrite(STDERR, 'Matches: ' . PHP_EOL);
            //fwrite(STDERR, print_r($matches, true) . PHP_EOL);
            //fwrite(STDERR, 'Hash: ' . $hash . PHP_EOL );
            //fwrite(STDERR, 'Defined functions: ' . print_r($this->functionCache, true) . PHP_EOL );
            //fwrite(STDERR, 'Registered Functions: ' . print_r($this->registeredFunctions, true) . PHP_EOL );

            // If we have an exact match, use that.
            if( array_key_exists( 0, $matches ) && count($matches[0]) == 1 ) {
                $match = $matches[0][0];

                $args = $match->args();
                $func = $match->call();
            }
            else if( $nMatches == 1 ) {
                // If there were no exact matches, but there was only one match, use that.
                $match = array_pop($matches);
                $match = array_pop($match);

                $args = $match->args();
                $func = $match->call();
            }
            else if ( $nMatches == 0 ) {
                // If there were no matches, try to generate one from a template
                // function.
                try {
                    $targs = [];
                    $info = $this->generateFunction( $name, $args, $targs, $hash );
                }
                catch( Exception $e ) {
                    grokit_error( 'Failed to lookup function ' . $fStr . ', no matching ' .
                        'registered functions and no compatible template.', $e );
                }

                return $info;
            }
            else {
                // There were multiple possible matches.
                // Aggregate the strings representing the possible matches and
                // then put out an error.

                $matchz = [];
                foreach( $matches as $matchList ) {
                    foreach( $matchList as $match ) {
                        $matchz[] = $match;
                    }
                }

                $matchStr = implode( PHP_EOL, $matchz );

                grokit_error( 'Failed to lookup function ' . $fStr . ', multiple possible' .
                    ' matches:' . PHP_EOL . $matchStr );
            }

            // If we've gotten this far, we should have the $args and $func variables
            // set with the particular function we are going to instantiate.

            // $args is just a list of type names, so lookup all the types first.
            $realArgs = [];
            foreach( $args as $n => $v ) {
                $realArgs[$n] = lookupType( $v );
            }

            // Perform the generation
            $targs = [];
            return $this->generateFunction( $oName, $args, $targs, $hash, $func );
        }

        public function lookupFunction( $name, array $args, array $targs = [], $fuzzy = true, $allowGenerate = true ) {
            // If the name of the function isn't namespaced, assume the base namespace
            // as long as we are doing fuzzy lookups with names.
            // If fuzzy is false, don't do this (mostly used for operators)
            if( ! self::IsNamespaced($name) && $fuzzy ) {
                $name = self::JoinNamespace('base', $name);
            }

            // Generate the hash for the function call.
            $hash = self::HashFunctionSignature( $name, $args, $targs );
            $has_targs = count($targs) > 0;

            // See if there is a cached function matching this hash, and if so,
            // use it.
            if( $this->isFunctionDefined( $hash ) ) {
                return $this->getDefinedFunction( $hash );
            }

            // If we've gotten this far, the function hasn't been cached (to our
            // knowledge), so we'll have to generate it.
            $info = null;

            // Only look through the registered concrete functions if there are no
            // template arguments.
            if( ! $has_targs && $this->functionRegistered( $name, $args ) ) {
                $info = $this->lookupConcreteFunction( $name, $args, $hash );
            }
            else if( $allowGenerate ) {
                $info = $this->generateFunction( $name, $args, $targs, $hash );
            } else {
                $fArgs = implode(', ', $args);
                grokit_error("Unable to lookup function {$name}({$fArgs})");
            }

            // Cache the function by both the original hash and the one provided
            // by the generator (if different)
            if( ! $this->isFunctionDefined( $hash ) ) {
                $this->functionCache[$hash] = $info;
            }
            if( ! $this->isFunctionDefined( $info->hash() ) ) {
                $this->functionCache[$info->hash()] = $info;
            }

            return $info;
        }

        /***** Static Members and Methods *****/

        // Mapping of binary operators to their form.
        // aata = Alpha x Alpha -> Alpha
        // aatb = Alpha x Alpha -> bool
        private static $binaryOperators = [
            // Alpha x Alpha -> Alpha operators
            '+' => 'aata',
            '-' => 'aata',
            '/' => 'aata',
            '*' => 'aata',
            '%' => 'aata',
            // Alpha x Alpha -> bool operators
            '&&' => 'aatb',
            '||' => 'aatb',
            '<' => 'aatb',
            '>' => 'aatb',
            '<=' => 'aatb',
            '>=' => 'aatb',
            '==' => 'aatb',
            '!=' => 'aatb',
            ];

        // Mapping of unary operators to their form.
        // ata = Alpha -> Alpha
        private static $unaryOperators = [
            '+' => 'ata',
            '-' => 'ata',
            '!' => 'ata',
            ];

        // Creates an operator generator function for an alpha-type operator with the given
        // type $alpha. The name of the operator is $op and the number of arguments is
        // $nArgs.
        // It is assumed that the C++ code has already been generated for this operator
        // as part of the type.
        public static function AlphaOperator( $op, $nArgs, DataType $alpha ) {
            $aVal = $alpha->value();

            switch( $nArgs ) {
            case 1:
                grokit_assert( array_key_exists( $op, self::$unaryOperators ),
                    'Attempted to generate invalid unary operator ' . $op . ' with alpha type ' .
                    $alpha );
                $form = self::$unaryOperators[$op];

                switch( $form ) {
                case 'ata':
                    $args = [ $alpha->value() ];
                    $call = function() use($alpha) {
                        return array(
                            'kind' => InfoKind::T_OP,
                            'input' => [ $alpha ],
                            'result' => $alpha,
                        );
                    };
                    break;
                }

                break;
            case 2:
                // Binary operator
                grokit_assert( array_key_exists( $op, self::$binaryOperators ),
                    'Attempted to generate invalid binary operator ' . $op . ' with alpha type ' .
                    $alpha );
                $form = self::$binaryOperators[$op];

                switch( $form ) {
                case 'aata':
                    $args = [ $aVal, $aVal ];
                    $call = function() use($alpha) {
                        return array (
                            'kind' => InfoKind::T_OP,
                            'input' => [ $alpha, $alpha ],
                            'result' => $alpha,
                        );
                    };
                    break;
                case 'aatb':
                    $args = [ $aVal, $aVal ];
                    $call = function() use($alpha) {
                        return array(
                            'kind' => InfoKind::T_OP,
                            'input' => [ $alpha, $alpha ],
                            'result' => lookupType('base::bool'),
                        );
                    };
                    break;
                }
            }

            declareOperator( $op, $args, $call );
        }

        public static function HashFunctionSignature( $name, array $args, array $targs = [] ) {
            $hasher = hash_init( 'sha256' );
            hash_update( $hasher, \strtoupper($name) );

            // Hash arguments
            hash_update( $hasher, '(' );
            foreach( $args as $n => $type ) {
                hash_update($hasher, $n);
                hash_update($hasher, $type->hash());
            }
            hash_update( $hasher, ')' );

            // If there are any template arguments, hash them too.
            if( count($targs) > 0 ) {
                hash_update( $hasher, '<' );
                hash_update( $hasher, hashComplex($targs) );
                hash_update( $hasher, '>' );
            }

            return hash_final($hasher);
        }

        // Namespace separators
        const NS_SEP     = '::';
        const PHP_NS_SEP = '\\';

        // Joins the names of namespaces properly.
        static public function JoinNamespace( $top, $bottom ) {
            if( $top != '' ) {
                return $top . self::NS_SEP . $bottom;
            }
            else {
                return $bottom;
            }
        }

        static public function JoinNamespacePHP( $top, $bottom ) {
            $top = rtrim($top, self::PHP_NS_SEP);
            $bottom = ltrim($bottom, PHP_NS_SEP);
            return $top . self::PHP_NS_SEP . $bottom;
        }

        // Determines whether or not a name is namespaced.
        static public function IsNamespaced( $name ) {
            return strpos($name, self::NS_SEP) !== false;
        }

        static public function SplitNamespace( $name ) {
            return explode(self::NS_SEP, $name);
        }

        static public function ExtractNamespace( $name ) {
            $parts = self::SplitNamespace($name);
            if( count($parts) == 1 ) {
                return '';
            }
            else {
                array_pop($parts);
                return implode(self::NS_SEP, $parts);
            }
        }

        private function CallGeneratorType( $name, &$args, &$buffer ) {
            $name_parts = explode(self::NS_SEP, $name);
            $func = self::PHP_NS_SEP . implode(self::PHP_NS_SEP, $name_parts);

            return self::CallGenerator( $name, $args, $func, $buffer );
        }

        // Calls a generation function, and wraps the generated code in its
        // namespace
        public function CallGenerator( $name, &$args, $func, &$buffer ) {
            $name_parts = explode(self::NS_SEP, $name);
            $ns_parts = array_map('strtoupper', explode(self::NS_SEP, $name, -1));
            $name_end = $name_parts[\count($name_parts) - 1];
            $ns = \implode(self::NS_SEP, $ns_parts);
            $name = self::JoinNamespace($ns, $name_end);

            if( is_string($func) ) {
                grokit_assert( function_exists($func), 'Unable to generate ' . $name .
                ', no function named ' . $func . ' exists' );
            }

            grokit_logic_assert( is_callable($func), 'Unable to generate ' . $name .
                ', value passed as function is not callable!');

            $preamble = '';
            $postamble = '';

            foreach( $ns_parts as $part ) {
                $preamble .= 'namespace ' . $part . '{' . PHP_EOL;
                $postamble .= PHP_EOL . '} // end namespace ' . $part . PHP_EOL;
            }

            ob_start();

            $res = call_user_func_array( $func, $args );

            $cont = ob_get_clean();

            if (is_typeinfo($res)) {
                return $res;
            }

            // Ensure the return value was correct and create the info object
            grokit_assert( is_array($res), 'Malformed return value from generator for ' .
                $name . ', got ' . gettype($res) . ' instead of an array' );

            grokit_assert( array_key_exists( 'kind', $res ),
                'Malformed return value from generator for ' . $name . ', no kind present.');

            $tName = $name;
            if( array_key_exists( 'name', $res ) ) {
                // They gave us back a name, so append it to the namespace.
                $tName = self::JoinNamespace($ns, $res['name']);
            }
            $res['name'] = $tName;

            if( array_key_exists( 'system_headers', $res ) ) {
                $sysHeaders = $res['system_headers'];

                foreach( $sysHeaders as $h ) {
                    $this->addHeader('<' . $h . '>');
                }
            }

            if( array_key_exists( 'user_headers', $res ) ) {
                $sysHeaders = $res['user_headers'];

                foreach( $sysHeaders as $h ) {
                    $this->addHeader('"' . $h . '"');
                }
            }

            if( array_key_exists( 'lib_headers', $res ) ) {
                $libHdrs = $res['lib_headers'];

                foreach($libHdrs as $h ) {
                    if( \count(explode('\\', $h)) == 1) {
                        // No library specified, assume current
                        $h = implode('\\', $ns_parts) . '\\' . $h;
                    }
                    $this->addLibHeader($h);
                }
            }

            $buffer .= $preamble;
            $buffer .= $cont;
            $buffer .= $postamble;

            if( array_key_exists( 'global_content', $res ) ) {
                $gCont = str_replace('@type', $tName, $res['global_content']);
                $buffer .= PHP_EOL . $gCont . PHP_EOL;
            }

            return $res;
        }

        // Global alias table
        private static $globalAlias = [];

        public function RegisterGlobalAlias( $name, $code, $info ) {
            grokit_logic_assert( ! self::IsGloballyAliased( $name ),
                'Attempting to doubly define alias ' . $name);

            self::$globalAlias[$name] = new GlobalAlias( $info, $code );
        }

        public static function IsGloballyAliased( $name ) {
            return array_key_exists($name, self::$globalAlias);
        }

        public static function ResolveGlobalAlias( $name ) {
            grokit_logic_assert( self::IsGloballyAliased( $name ),
                'Attempting to reference unknown alias ' . $name);

            // See if we have already generated code for this type in this
            // file.
            if( self::$instance->typeDefined( $name ) ) {
                return self::$instance->lookupType( $name );
            }

            // We haven't, so look it up in the global alias table and echo
            // the code and register it as defined.
            $cache = self::$globalAlias[$name];

            echo $cache->code();
            self::$instance->defineType( $name, $cache->info() );

            return $cache->info();
        }

        // Table of types declared by libraries. These are essentially short
        // names for complex types.
        private static $declaredTypes = [];

        public static function DeclareType($name, $tName, array $targs) {
            $name = \strtoupper($name);
            $tName = \strtoupper($tName);

            grokit_error_if(array_key_exists($name, self::$declaredTypes),
                'Attempting to multiply declare type ' . $name);

            //fwrite(STDERR, "TYPEDEF: name({$name}) type({$tName}) targs(" . squashToString($targs) . ")" . PHP_EOL);

            self::$declaredTypes[$name] = [ 'name' => $tName, 'targs' => $targs ];
        }

        public static function HasTypeDeclaration( $name ) {
            $name = \strtoupper($name);
            return array_key_exists($name, self::$declaredTypes);
        }

        private static function GetTypeDeclaration_Helper($name, $targs, $stack) {
            $name = \strtoupper($name);
            grokit_logic_assert( !in_array($name, $stack),
                "Cicular type definition detected: type($name), stack(" . squashToString($stack) . ")" );
            grokit_logic_assert( self::HasTypeDeclaration($name),
                'Attempting to get unknown type declaration ' . $name);

            $stack[] = $name;

            $decltype = self::$declaredTypes[$name];

            if( self::HasTypeDeclaration($decltype['name']) ) {
                $decltype = self::GetTypeDeclaration_Helper($decltype['name'], $decltype['targs'], $stack);
            }

            // Replace default arguments with those given higher up
            $decltype['targs'] = array_replace($decltype['targs'], $targs);

            return $decltype;
        }

        public static function GetTypeDeclaration($name, $targs) {
            return self::GetTypeDeclaration_Helper($name, $targs, []);
        }

        // Stack for saving our state when starting new files
        private static $stack = [];

        // Singleton pattern
        private static $instance = null;

        public static function & GetLibraryManager() {
            if( self::$instance === null ) {
                self::$instance = new LibraryManager();

                // Set up boolean datatype
                $boolHash = hash( 'sha256', 'BASE::BOOL' );
                $extras = [
                    'binary_operators' => [ '||', '&&' ],
                    'unary_operators' => [ '!' ],
                    'properties' => ['_primative_'],
                    'describe_json' => DescribeJson('bool'),
                    ];
                $boolDT = new DataType( $boolHash, 'BASE::BOOL', 'bool', $extras, [[]] );

                self::$instance->defineType( $boolHash, $boolDT );

                // Set up null datatype
                $nullHash = hash( 'sha256', 'BASE::NULL' );
				$altNullHash = hash( 'sha256', 'BASE::GROKITNULL' );
                $nullExtras = [ 'properties' => [ 'null' ] ];
                $nullDT = new DataType( $nullHash, 'BASE::NULL', 'GrokitNull', $nullExtras, [[]] );

                self::$instance->defineType( $nullHash, $nullDT );
				self::$instance->defineType( $altNullHash, $nullDT );
            }

            return self::$instance;
        }

        // Make a copy of the current library manager and save it on the stack.
        public static function Push() {
            //fwrite(STDERR, 'Push' . PHP_EOL);
            $curInst = & self::GetLibraryManager();

            self::$stack[] = clone $curInst;
        }

        // Replace the current library manager with the most recent saved copy
        public static function Pop() {
            //fwrite(STDERR, 'Pop' . PHP_EOL);

            grokit_logic_assert( count(self::$stack) > 0,
                'Too many pops on LibraryManager');

            $inst = array_pop(self::$stack);
            self::$instance = $inst;
        }

        public static function GetBuffer() {
            $ret = "";
            foreach( self::$instance->headers as $h ) {
                $ret .= '#include ' . $h . PHP_EOL;
            }
            foreach( self::$instance->libHeaders as $h ) {
                $ret .= $h . PHP_EOL;
            }
            return $ret . self::$instance->buffer;
        }
    }

    // Hash a list of TemplateArgs or TypeInfos
    function hashArgList( &$args ) {
        return hashComplex($args);
        /*
        foreach( $args as $name => &$arg ) {
            hash_update( $hasher, $name );
            hash_update( $hasher, $arg->hash() );
        }

        return hash_final( $hasher );
         */
    }

} // end namespace grokit

namespace {

    // All of the functions that serve as an interface to the library manager

    function declareTypeGlobal( $ns, $name, $tName, array $targs ) {
        $ns_parts = explode(\grokit\LibraryManager::PHP_NS_SEP, $ns);
        $cNS = implode(\grokit\LibraryManager::NS_SEP, $ns_parts);

        $fName = \grokit\LibraryManager::JoinNamespace( $cNS, $name );

        \grokit\LibraryManager::DeclareType($fName, $tName, $targs);
    }

    // General lookup function, used by the other functions to perform lookups on
    // their various types.
    function _lookupType( $name, array &$args, $alias = null, $hash = null ) {
        $libman = & \grokit\LibraryManager::GetLibraryManager();

        if( $alias !== null ) {
            ob_start();
        }

        $info = $libman->lookupType($name, $args, $hash);

        if( $alias !== null ) {
            $code = ob_get_flush();

            \grokit\LibraryManager::RegisterGlobalAlias( $alias, $code, $info);
        }

        return $info;
    }

    /**
     * Determines if a type has been defined depending on a set of
     * characteristics that determine uniqueness.
     *
     * @param $characteristics an array containing uniqueness characteristics
     * for the type.
     * @param $hash output parameter for the unique hash for the type
     * @param $type output parameter for the type information object for the
     * type, if it is defined.
     *
     * @returns true if the type was defined, false otherwise
     */
    function typeDefined( $characteristics, &$hash, &$type ) {
        $libman = & \grokit\LibraryManager::GetLibraryManager();
        $hash = \grokit\hashComplex($characteristics);

        if($libman->typeDefined($hash)) {
            $type = $libman->getDefinedType($hash);
            return true;
        }

        return false;
    }

    function lookupType( $name, array $t_args = null, $alias = null ) {
        if (is_datatype($name))
            return $name->lookup();

        $t_args = is_null($t_args) ? [] : $t_args;
        $args = [ $t_args ];
        return _lookupType( $name, $args, $alias );
    }

    function lookupAlias( $name ) {
        // Perform local lookup first to see if we've already defined the type.
        $libman = & \grokit\LibraryManager::GetLibraryManager();
        if( $libman->typeDefined( $name ) ) {
            return $libman->lookupType( $name );
        }

        // It's not defined locally, so look it up in the global alias table.
        $cache = \grokit\LibraryManager::ResolveGlobalAlias( $name );

        echo $cache->code();
        return $cache->info();
    }

    function declareSynonym( $syn, $base ) {
        $libman = & \grokit\LibraryManager::GetLibraryManager();
        $libman->addAlias($syn, $base);
    }

    // Determine if it is possible to implicitly convert from sourceType to destType
    function canConvert( $sourceType, $destType ) {
        if( $sourceType == $destType )
            return true;

        // See if a constructor for destType exists from sourceType
        if( functionRegistered( $destType, [ $sourceType ], true ) ) {
            return true;
        }

        return false;
    }

    // Perform an implicit conversion to convert $expression to $type.
    function convertExpression( grokit\ExpressionInfo &$expression, $type, $source = null ) {
        if( $source === null )
            $source = $expression->source();
        grokit_logic_assert( canConvert( $expression->type(), $type),
            'Unable to convert expression from type ' . $expression->type() . ' to type ' . $type);

        if( $expression->type() == $type ){
            return $expression;
        }

        $func = lookupFunction( strval($type), [ $expression->type() ], [] );
        //fwrite(STDERR, "Convert: source(" . $expression->type() . ") dest(" . $type . ")" . PHP_EOL);
        //fwrite(STDERR, print_r($func, true));
        $info = $func->apply([$expression], $source);

        if( $info->is_const() ) {
            $info->makeConstant();
        }

        return $info;
    }

    function functionRegistered( $name, array $args, $exact = false ) {
        $libman = & \grokit\LibraryManager::GetLibraryManager();

        return $libman->functionRegistered( $name, $args, $exact );
    }

    function functionDefined( $name, array $args, array $targs = null ) {
        $targs = is_null($targs) ? [] : $targs;
        $hash = \grokit\LibraryManager::HashFunctionSignature( $name, $args, $targs );

        $libman = & \grokit\LibraryManager::GetLibraryManager();

        return $libman->isFunctionDefined( $hash );
    }

    function lookupFunction( $name, array $args, array $t_args = null, $fuzzy = true, $allowGenerate = true ) {
        $t_args = is_null($t_args) ? [] : $t_args;
        $libman = & \grokit\LibraryManager::GetLibraryManager();

        return $libman->lookupFunction( $name, $args, $t_args, $fuzzy, $allowGenerate );
    }

    function lookupOperator( $name, $args ) {
        $libman = & \grokit\LibraryManager::GetLibraryManager();

        return $libman->lookupFunction( $name, $args, [], false );
    }

    function declareFunctionGlobal( $ns, $name, array $args, callable $call ) {
        // $ns is a PHP namespace, convert it to a C++ namespace.
        $ns_parts = explode(\grokit\LibraryManager::PHP_NS_SEP, $ns);
        $cNS = implode(\grokit\LibraryManager::NS_SEP, $ns_parts);

        $name = \grokit\LibraryManager::JoinNamespace( $cNS, $name );

        $libman = & \grokit\LibraryManager::GetLibraryManager();

        $libman->registerFunction( $name, $args, $call );
    }

    function declareOperator( $name, array $args, callable $call ) {
        $libman = & \grokit\LibraryManager::GetLibraryManager();

        $libman->registerFunction( $name, $args, $call );
    }

    function lookupResource( $name, $t_args, $alias = null ) {
        $args = [ $t_args ];
        $info = _lookupType( $name, $args, $alias );
        grokit_assert( $info->isResource(),
            'Tried lookup up ' . $name . ' as a Resource, got a ' . $info->kind() . ' instead');

        return $info;
    }

    // The following functions would be called by the specifications of the GLA/GT/GF via the
    // Spec's apply() method.
    function lookupGLA( $name, $t_args, $input, $output, $sargs = [], $alias = null, $hash = null ) {
        $args = [ $t_args, $input, $output, $sargs ];
        $info = _lookupType( $name, $args, $alias, $hash );
        grokit_assert( $info->isGLA(),
            'Tried looking up ' . $name . ' as a GLA, got a ' . $info->kind() . ' instead');

        return $info;
    }

    function lookupGT( $name, $t_args, $input, $output, $sargs = [], $alias = null ) {
        $args = [ $t_args, $input, $output, $sargs ];
        $info = _lookupType( $name, $args, $alias );
        grokit_assert( $info->isGT(),
            'Tried looking up ' . $name . ' as a GT, got a ' . $info->kind() . ' instead');

        return $info;
    }

    function lookupGF( $name, $t_args, $input, $alias = null ) {
        $args = [ $t_args, $input ];
        $info = _lookupType($name, $args, $alias);
        grokit_assert( $info->isGF(),
            'Tried looking up ' . $name . ' as a GF, got a ' . $info->kind() . ' instead');

        return $info;
    }

    function lookupGIST( $name, $t_args, $output, $sargs, $alias = null, $hash = null ) {
        $args = [ $t_args, $output, $sargs ];
        $info = _lookupType( $name, $args, $alias, $hash );
        grokit_assert( $info->isGIST(),
            'Tried looking up ' . $name . ' as a GIST, got a ' . $info->kind() . ' instead');

        return $info;
    }

    function lookupGSE( $name, $t_args, $alias = null ) {
        $args = [ $t_args ];
        $info = _lookupType( $name, $args, $alias );
        grokit_assert($info->isGSE(),
            'Tried to lookup ' . $name . ' as a GSE, got a ' . $info->kind() . ' instead');

        return $info;
    }

    function lookupGI( $name, $t_args, $output, $alias = null ) {
        $args = [ $t_args, $output ];
        $info = _lookupType( $name, $args, $alias );
        grokit_assert( $info->isGI(),
            'Tried looking up ' . $name . ' as a GI, got a ' . $info->kind() . ' instead');

        return $info;
    }

    // Returns the correct type definition for hashed types.
    function hashName( $name ) {
        $hasher = hash_init( 'sha256' );
        hash_update( $hasher, $name );
        return '0x' . substr(hash_final($hasher), 0, 16) . 'ULL';
    }
}

?>
