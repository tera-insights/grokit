<?php

// Copyright 2013 Tera Insights, LLC. All Rights Reserved.

/*
 * This file contains the definitions of classes that represent types.
 */

/*
 *  All types require the following values in the array they return:
 *      - 'kind' :
 *              The kind of type they are. (should be one of the constants
 *              in TypeInfo)
 *
 *  The following values are optional for all types:
 *      - 'name' :
 *              The actual name of the generated type. By default this is
 *              assumed to be the name of the function that was called.
 *              If given, this should not include the overall namespace that
 *              is generated for the type, as that will be applied by the
 *              system.
 *      - 'system_headers'
 *              A list of any system headers required by the type. These
 *              should not include the angle brackets.
 *      - 'user_headers'
 *              A list of any user headers required by the type. These should
 *              not include the quotes.
 *      - 'extra_properties'
 *              An associative array of extra properties that the TypeInfo should
 *              make available. These can be accessed by the has() and get() methods
 *              of the TypeInfo class.
 *      - 'complex'
 *              String that denotes the type of the iterator to be used for this type. Inside
 *              the string, any occurrence of the @ symbol will be replaced with the
 *              name of the type.
 *              If absent or false, the default will be used.
 *
 *  GIs require the following extra properties:
 *      - 'output' :
 *              A list of TypeInfo values that designate the types that the GI
 *              produces as output.
 *
 *  GLAs require the following extra required properties:
 *      - 'input' :
 *              A list of TypeInfo values that designate thetypes that the GI
 *              expects to receive as input.
 *      - 'output' :
 *              A list of TypeInfo values that designate the types that the GI
 *              produces as output.
 *      - 'result_type':
 *              An array containing the set of supported result types.
 *              The result types supported by the system are:
 *              [ 'single', 'multi', 'fragment', 'state' ]
 */

namespace grokit {

    function squashToString($args, $lb = '[', $rb = ']') {
        if( is_array($args) ) {
            return $lb . implode(', ', array_map(__FUNCTION__, $args)) . $rb;
        }
        else if( is_object($args) ) {
            return strval($args);
        }
        else {
            // Literal value
            return $args;
        }
    }

    function squash($args) {
        if( is_array($args) ) {
            return array_map('\\grokit\\squash', $args);
        }
        else if( is_object($args) ) {
            return strval($args);
        }
        else {
            // Literal value
            return $args;
        }
    }

    function summarize($args) {
        if( is_array($args) ) {
            return array_map(__FUNCTION__, $args);
        }
        else if( is_object($args) ) {
            return $args->summary();
        }
        else {
            // Literal value
            return $args;
        }
    }

    /*
     * Base class for information about types with different kinds
     */
    abstract class InfoKind {
        // Constants for valid values of kind
        const T_TYPE    = 'TYPE';
        const T_GLA     = 'GLA';
        const T_GF      = 'GF';
        const T_GT      = 'GT';
        const T_GIST    = 'GIST';
        const T_GI      = 'GI';
        const T_GSE     = 'GSE';

        const T_RESOURCE = 'RESOURCE';

        const T_FUNC    = 'FUNCTION';
        const T_OP      = 'OPERATOR';

        // Determines what kind of data type this is (e.g. standard type, GLA, GF, etc.)
        private $kind;

        // Constructor
        public function __construct( $kind ) {
            $this->kind = $kind;
        }

        public function kind() { return $this->kind; }

        public function isDataType() { return $this->kind == self::T_TYPE; }
        public function isGLA() { return $this->kind == self::T_GLA; }
        public function isGF() { return $this->kind == self::T_GF; }
        public function isGT() { return $this->kind == self::T_GT; }
        public function isGIST() { return $this->kind == self::T_GIST; }
        public function isGI() { return $this->kind == self::T_GI; }
        public function isGSE() { return $this->kind == self::T_GSE; }

        public function isResource() { return $this->kind == self::T_RESOURCE; }

        public function isFunction() { return $this->kind == self::T_FUNC; }
        public function isOperator() { return $this->kind == self::T_OP; }

        public function summary() {
            return [ 'kind' => $this->kind ];
        }
    }

    /*
     * This is the base class for all of the other type infos.
     */
    abstract class TypeInfo extends InfoKind {
        /****** Class Members ******/
        // The name of the type. If the type was templated, this is the name of
        // the template.
        private $name;

        // The C++ type that implements this type.
        private $value;

        // Boolean properties of the type
        private $properties = [];

        // Extra attributes of the type
        private $extras = [];

        // Hash that should uniquely identify the type.
        private $hash;

        // A list of libraries required by the type
        private $libraries = [];

        private $targs = [];

        /****** Class Methods ******/

        // Summary information
        public function summary() {
            $ret = parent::summary();

            $ret['name'] = $this->name;
            $ret['value'] = $this->value;
            $ret['properties'] = $this->properties;
            $ret['extras'] = $this->extras;
            $ret['hash'] = $this->hash;
            $ret['libraries'] = $this->libraries;
            $ret['targs'] = squash($this->targs);

            return $ret;
        }

        // Getters for standard properties
        public function name() { return $this->name; }
        public function value() { return $this->value; }
        public function hash() { return $this->hash; }
        public function libraries() { return $this->libraries; }
        public function template_args() { return $this->targs; }

        // Return a truncated version of the hash suitable for inclusion in
        // the C++ source code as a 64-bit integer.
        public function cHash() {
            return '0x' . substr($this->hash, 0, 16) . 'ULL';
        }

        // Constructor
        // Created from values in an associative array
        public function __construct( $kind, $hash, $name, $value, array $args, array $targs )
        {
            parent::__construct( $kind );
            $this->name = $name;
            $this->value = $value;
            $this->hash = $hash;
            $this->targs = $targs;

            if( array_key_exists('properties', $args) ) {
                $this->properties = $args['properties'];
            }

            $this->extras = get_first_key_default($args, ['extra', 'extras'], []);

            if( array_key_exists('libraries', $args) ) {
                $this->libraries = $args['libraries'];
            }
        }

        public function __toString() {
            return $this->value;
        }

        // Methods to query for extra properties
        public function has( $name ) {
            return array_key_exists($name, $this->extras);
        }

        public function get( $name ) {
            if( $this->has($name) ) {
                return $this->extras[$name];
            }

            grokit_error('Attempted to access undefined extra attribute ' . $name .
                ' on TypeInfo for ' . $this->kind() . ' ' . $this->name );
        }

        // Query for boolean properties
        public function is( $name ) {
            return in_array($name, $this->properties) ? TRUE : FALSE;
        }

        public function extras() { return $this->extras; }
        public function properties() { return $this->properties; }

    }

    /*
     * Represents resources used by other types, like generated constant states
     * for GLAs.
     */
    class Resource_Info extends TypeInfo {
        private $configurable = false;
        private $mutable = false;

        public function __construct( $hash, $name, $value, array $args, array $oArgs ) {
            parent::__construct(InfoKind::T_RESOURCE, $hash, $name, $value, $args, $oArgs[0]);

            $this->configurable = get_default($args, 'configurable', false);
            $this->mutable = get_default($args, 'mutable', false);
        }

        public function configurable() { return $this->configurable; }
        public function mutable() { return $this->mutable; }
    }

    // Specialization of TypeInfo for standard types.
    // Just overrides the constructor to pass in its kind.
    class DataType extends TypeInfo {
        // String containing the type of the iterator
        private $iterator;

        // If not null, this type requires the dictionary with this name.
        private $dictionary = null;

        private $fixedSize = true;

        // Mapping of method name to list of methods with that name.
        private $methods = [];

        // Whether or not the type needs to be specifically destroyed upon
        // deallocation.
        private $destroy = false;

        // Functions that can be used to produce descriptions of the type in
        // various formats.
        private $describers = [];

        // Summary information
        public function summary() {
            $ret = parent::summary();

            $ret['iterator'] = $this->iterator;
            $ret['dictionary'] = $this->dictionary;
            $ret['fixed_size'] = $this->fixedSize;
            $ret['methods'] = summarize($this->methods);
            $ret['destroy'] = $this->destroy;

            return $ret;
        }

        // Getters for standard properties
        public function iterator() { return $this->iterator; }
        public function complex() { return $this->iterator !== false; }

        public function reqDictionary() { return $this->dictionary !== null; }
        public function dictionary() { return $this->dictionary; }

        public function isFixedSize() { return $this->fixedSize; }

        public function destroy() { return $this->destroy; }

        public function describer($type) {
            if( array_key_exists($type, $this->describers) ) {
                $desc = $this->describers[$type];
                $myobj = $this;
                return function( $var, $dType = null ) use ($desc, $myobj) {
                    if( ! is_null($dType) )
                        $desc($var, $dType);
                    else
                        $desc($var, $myobj);
                };
            } else {
                return null;
            }
        }

        // Looks up the type again to ensure it exists.
        public function lookup() { return lookupType($this->name(), $this->template_args()); }

        public function __construct( $hash, $name, $value, array $args, array $origArgs ) {
            parent::__construct(InfoKind::T_TYPE, $hash, $name, $value, $args, $origArgs[0]);

            $iterator = 'ColumnIterator<' . $value . '>';
            if( array_key_exists('complex', $args) ) {
                $iter = $args['complex'];

                if( $iter !== false ) {
                    $iterator = str_replace( '@type', $this->value(), $iter );
                }
            }

            $this->iterator = $iterator;

            if( array_key_exists('destroy', $args) ) {
                $destroy = $args['destroy'];

                if( is_bool($destroy) ) {
                    $this->destroy = $destroy;
                }
                else {
                    grokit_error("Expected boolean value for 'destroy' attribute of datatype " . $this->value());
                }
            }

            if( array_key_exists( 'dictionary', $args ) ) {
                $this->dictionary = $args['dictionary'];
            }

            if( array_key_exists( 'fixed_size', $args ) ) {
                $this->fixedSize = $args['fixed_size'];
            }

            // Deal with operators defined by the type.
            if( array_key_exists( 'unary_operators', $args ) ) {
                foreach( $args['unary_operators'] as $op ) {
                    LibraryManager::AlphaOperator( $op, 1, $this );
                }
            }
            if( array_key_exists( 'binary_operators', $args ) ) {
                foreach( $args['binary_operators'] as $op ) {
                    LibraryManager::AlphaOperator( $op, 2, $this );
                }
            }

            if( array_key_exists( 'constructors', $args ) ) {
                foreach( $args['constructors'] as $cstr ) {
                    $cArgs = get_first_key($cstr, ['args', 0] );
                    $cDet = get_first_key_default($cstr, ['deterministic', 1], true);
                    $myType = $this->value();
                    $myName = end(explode('::', $myType));
                    $cName = get_first_key_default($cstr, ['c_name', 2], $myName);

                    $callback = function($args, $t_args = []) use($cArgs, $cDet, $myType, $cName) {
                        $myArgs = array_map('lookupType', $cArgs);
                        $myRet = lookupType($myType);

                        $retVal = [
                            'kind'      => 'FUNCTION',
                            'input'     => $myArgs,
                            'result'    => $myRet,
                            'deterministic' => $cDet,
                            ];

                        if( $cName !== null ) {
                            $retVal['name'] = $cName;
                        }

                        return $retVal;
                    };

                    declareFunctionGlobal('', $myType, $cArgs, $callback);
                }
            }

            if( array_key_exists( 'functions', $args ) ) {
                foreach( $args['functions'] as $fnct ) {
                    $fName = get_first_key($fnct, ['name', 0]);
                    $fArgs = get_first_key($fnct, ['args', 1]);
                    $fRet = get_first_key($fnct, ['result', 2]);
                    $fDet = get_first_key_default($fnct, ['deterministic', 3], false);
                    $fGlobal = get_first_key_default($fnct, ['global', 4], false);
                    $cName = get_first_key_default($fnct, ['c_name', 5], null);

                    foreach( $fArgs as &$arg ) {
                        $arg = str_replace('@type', $this->value(), $arg);
                    }
                    $fRet = str_replace('@type', $this->value(), $fRet);

                    $callback = function($args, $t_args = []) use ($fArgs, $fRet, $fDet, $cName, $fGlobal) {
                        $myArgs = array_map('lookupType', $fArgs);
                        $myRet = lookupType($fRet);

                        $retVal = [
                            'kind'      => 'FUNCTION',
                            'input'     => $myArgs,
                            'result'    => $myRet,
                            'deterministic' => $fDet,
                            'global'    => $fGlobal,
                            ];

                        if( $cName !== null ) {
                            $retVal['name'] = $cName;
                        }

                        return $retVal;
                    };

                    $fNS = $fGlobal ? 'BASE' : LibraryManager::ExtractNamespace($this->value());
                    $fName = LibraryManager::JoinNamespace($fNS, $fName);

                    declareFunctionGlobal( '', $fName, $fArgs, $callback );
                }
            }

            if( array_key_exists( 'methods', $args ) ) {
                foreach( $args['methods'] as $method ) {
                    $mName = get_first_key($method, [ 'name', 0 ] );
                    $mArgs = get_first_key($method, [ 'args', 1 ] );
                    $mRet = get_first_key($method, [ 'result', 2 ] );
                    $mDet = get_first_key_default($method, ['deterministic', 3], false);

                    if( $mRet == '@type' ) {
                        $mRet = $this->value();
                    }

                    foreach ( $mArgs as &$mArg ) {
                        if( $mArg == '@type' ) {
                            $mArg = $this->value();
                        }
                    }

                    //fwrite(STDERR, 'Defining method ' . $this->value() . '->' . $mName .
                        //'(' . implode(', ', $mArgs ) . ') : ' . $mRet . PHP_EOL );

                    $info = new MethodInfo($mName, $mArgs, $mRet, $mDet);

                    if( !array_key_exists( $mName, $this->methods ) ) {
                        $this->methods[$mName] = [ $info ];
                    }
                    else {
                        foreach( $this->methods[$mName] as $cm ) {
                            if( $info == $cm ) {
                                grokit_error('Attempting to multiply define method ' . $this->value() . '->' . $mName .
                                '(' . implode(', ', $mArgs) . ')' );
                            }
                        }
                        $this->methods[$mName][] = $info;
                    }
                }
            }

            if( array_key_exists( 'describe_json', $args ) ) {
                $this->describers['json'] = $args['describe_json'];
            }
        }

        public function lookupMethod ( $name, array $args ) {
            grokit_assert( array_key_exists( $name, $this->methods ),
                'No method registered with the name ' . $this->value() . '->' . $name );

            $candidates = & $this->methods[$name];

            // Matches is a mapping from the score to the candidate MethodInfo
            // We keep track of all possible matches in case we have multiple possibilities,
            // and we may in the future be able to print nice error messages.
            $matches = [];
            $nMatches = 0;

            $fStr = $this->value() . '->' . $name . '('. implode(', ', $args) . ')';
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
                'Got more than one exact match for method ' . $fStr );

            //fwrite(STDERR, 'Matches: ' . PHP_EOL);
            //fwrite(STDERR, print_r($matches, true) . PHP_EOL);
            //fwrite(STDERR, 'Defined functions: ' . print_r($this->functionCache, true) . PHP_EOL );
            //fwrite(STDERR, 'Registered Functions: ' . print_r($this->registeredFunctions, true) . PHP_EOL );

            // If we have an exact match, use that.
            if( array_key_exists( 0, $matches ) && count($matches[0]) == 1 ) {
                $match = $matches[0][0];
            }
            else if( $nMatches == 1 ) {
                // If there were no exact matches, but there was only one match, use that.
                $match = array_pop($matches);
                $match = array_pop($match);
            }
            else if ( $nMatches == 0 ) {
                grokit_error('Failed to lookup method ' . $fStr . ', no possible matches.');
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

                grokit_error( 'Failed to lookup method ' . $fStr . ', multiple possible' .
                    ' matches:' . PHP_EOL . $matchStr );
            }

            return $match;
        }
    }

    class GeneralizedObject extends TypeInfo {
        use ArgumentChecker;

        private $req_states = [];

        // The type of the generated state (null if none)
        private $gen_state = null;
        // Whether or not the GO is configurable with JSON
        private $configurable = false;

        private $gen_states = [];       // DEPRECATED
        private $constructor_args = []; // DEPRECATED

        public function __construct($kind, $hash, $name, $value, array $args, array $targs ) {
            parent::__construct($kind, $hash, $name, $value, $args, $targs );

            if( array_key_exists( 'required_states', $args ) ) {
                $this->req_states = $args['required_states'];
            }

            if( array_key_exists('generated_state', $args) ) {
                $this->gen_state = $args['generated_state'];
            }

            $this->configurable = get_default($args, 'configurable', false);

            if( array_key_exists( 'generated_states', $args ) ) {
                grokit_warning('"generated_states" type information parameter is deprecated. Please use "generated_state" instead.');
                $this->gen_states = ensure_unique_names($args['generated_states'], 'gen_state');
                if( is_null($this->gen_state) && \count($this->gen_states) == 1 ) {
                    $this->gen_state = array_get_index($this->gen_states, 0);
                }
            }

            if( array_key_exists( 'constructor_args', $args ) ) {
                grokit_warning('"constructor_args" type information parameter is deprecated.');
                $this->constructor_args = ensure_unique_names($args['constructor_args'], 'const_arg');
            }
        }

        public function summary() {
            $ret = parent::summary();

            $ret['req_states'] = squash($this->req_states);
            $ret['gen_state'] = summarize($this->gen_state);
            $ret['configurable'] = $this->configurable;

            return $ret;
        }

        // Getters
        public function req_states() { return $this->req_states; }
        public function gen_states() { return $this->gen_states; }

        public function configurable() { return $this->configurable; }
        public function state() { return $this->gen_state; }
        public function has_state() { return !is_null($this->gen_state); }

        // DEPRECATED
        public function num_states() { return count($this->req_states) + count($this->gen_states); }
        public function states() { return array_merge( $this->req_states, $this->gen_states ); }
        public function constructor_args() { return $this->constructor_args; }
    }

    trait ArgumentChecker {
        // Checks to see if an array of types is exactly equal to a given
        // array of types.
        // The $type parameter is used in error messages
        protected function CheckExact( array &$expected, array &$given, $type ) {
            $nExpected = count($expected);
            $nGiven = count($given);

            grokit_assert( $nExpected == $nGiven,
                'Unable to apply ' . $this . ' to given ' . $type . ': Expected ' .
                $nExpected . ' ' . $type . ', received ' . $nGiven );

            reset($expected);   reset($given);
            foreach( range(0, $nExpected-1) as $i ) {
                $cExpected = current($expected);
                $cGiven = current($given);

                if( is_null($cGiven) ) {
                    // If the given attribute is null, it's blank and thus
                    // automatically compatible.
                    continue;
                }

                grokit_assert( $cExpected == $cGiven,
                    'Unable to apply ' . $this . ' to given ' . $type . 's: ' .
                    '' . $type . ' at position ' . $i . ' of type ' . $cGiven . ' does not match ' .
                    'expected type ' . $cExpected );

                next($expected);    next($given);
            }
        }

        // Checks to see if an array of expressions has compatible types to a given
        // array of types, and converts them if they are.
        // The $type parameter is used in error messages.
        protected function CheckFuzzy( array &$expected, array &$exprs, $type ) {
            //fwrite(STDERR, "CheckFuzzy: " . print_r($exprs,true) . PHP_EOL);
            $nGiven = \count($exprs);
            $nExpected = \count($expected);

            grokit_assert( $nExpected == $nGiven,
                'Unable to apply ' . $this . ' to given ' . $type . 's: Expected ' .
                $nExpected . ' ' . $type . 's, received ' . $nGiven );

            if( $nGiven == 0 )
                return;

            $expectKeys = array_keys( $expected );

            reset($expected);   reset($exprs);
            foreach( range(0, $nExpected-1) as $i ) {
                $cExpected = current($expected);
                $cGiven = current($exprs);
                $cGivenType = is_datatype($cGiven) ? $cGiven : $cGiven->type();

                grokit_assert( canConvert( $cGivenType, $cExpected ),
                    'Unable to apply ' . $this . ' to given ' . $type . 's: ' .
                    '' . $type . ' at position ' . $i . ' of type ' . $cGiven->type() . ' not compatible with ' .
                    'expected type ' . $cExpected );

                if( is_expression( $cGiven ) )
                    $exprs[key($exprs)] = convertExpression( $cGiven, $cExpected );

                next($expected);    next($exprs);
            }
        }
    }

    class GI_Info extends GeneralizedObject {

        private $output = [];

        public function __construct( $hash, $name, $value, array $args, $oArgs ) {
            parent::__construct(InfoKind::T_GI, $hash, $name, $value, $args, $oArgs[0]);

            grokit_assert( array_key_exists( 'output', $args ),
                'No outputs declared for ' . $this );

            $this->output = $args['output'];
        }

        public function summary() {
            $ret = parent::summary();

            $ret['output'] = squash($this->output);

            return $ret;
        }

        // Getters
        public function output() {
            return $this->output;
        }

        /*
         * $outputs should be an array of TypeInfo objects giving the types of
         * the given outputs.
         */
        public function apply( array $outputs ) {
            $this->CheckExact( $this->outputs, $outputs, 'output' );

            // If we've gotten this far, then everything is compatible.
            // Return ourselves as the information.
            return $this;
        }

        public function lookup() {
            $outputs = [];
            foreach( $this->output as $name => $type ) {
                $outputs[$name] = is_type($type) ? $type->lookup() : $type;
            }
            return lookupGI($this->name(), $this->template_args(), $outputs);
        }
    }

    class GLA_Info extends GeneralizedObject {

        private $output = [];
        private $input = [];
        private $result_type;
        private $iterable = false;
        private $finalize_as_state = false;
        private $post_finalize = false;
        private $pre_chunk = false;
        private $chunk_boundary = false;
        private $intermediates = false;

        public function __construct( $hash, $name, $value, array $args, array $oArgs ) {
            $args['req_states'] = $oArgs[3];

            parent::__construct(InfoKind::T_GLA, $hash, $name, $value, $args, $oArgs[0]);

            grokit_assert( array_key_exists( 'output', $args ),
                'No outputs declared for ' . $this );

            $this->output = $args['output'];

            grokit_assert( array_key_exists( 'input', $args ),
                'No inputs declared for ' . $this );

            $this->input = $args['input'];

            grokit_assert( array_key_exists( 'result_type', $args ),
                'No result type declared for ' . $this );

            $this->result_type = $args['result_type'];
            if( ! is_array($this->result_type) ) {
                $this->result_type = [ $this->result_type ];
            }

            if( array_key_exists( 'iterable', $args ) ) {
                $this->iterable = $args['iterable'];
                if( $this->iterable ) {
                    grokit_assert( $this->has_state(),
                        'GLA ' . $this . ' marked as iterable but has no constant state');
                }
            }

            if( array_key_exists( 'finalize_as_state', $args ) ) {
                $this->finalize_as_state = $args['finalize_as_state'];
            }

            if( array_key_exists( 'chunk_boundary', $args ) ) {
                $this->chunk_boundary = $args['chunk_boundary'];
            }
            if( array_key_exists( 'pre_chunk', $args ) ) {
                $this->pre_chunk = $args['pre_chunk'];
            }

            if( array_key_exists( 'intermediates' , $args ) ) {
                $this->intermediates = $args['intermediates'];
            }

            if( array_key_exists( 'post_finalize', $args ) ) {
                $this->post_finalize = $args['post_finalize'];
            }
        }

        public function summary() {
            $ret = parent::summary();

            $ret['input'] = squash($this->input);
            $ret['output'] = squash($this->output);
            $ret['result_type'] = $this->result_type;
            $ret['iterable'] = $this->iterable;
            $ret['finalize_as_state'] = $this->finalize_as_state;
            $ret['post_finalize'] = $this->post_finalize;
            $ret['chunk_boundary'] = $this->chunk_boundary;
            $ret['intermediates'] = $this->intermediates;

            return $ret;
        }

        // Getters
        public function output() { return $this->output; }
        public function input() { return $this->input; }
        public function result_type() { return $this->result_type; }
        public function iterable() { return $this->iterable; }
        public function finalize_as_state() { return $this->finalize_as_state; }
        public function post_finalize() { return $this->post_finalize; }
        public function pre_chunk() { return $this->pre_chunk; }
        public function chunk_boundary() { return $this->chunk_boundary; }
        public function intermediates() { return $this->intermediates; }

        /*
         * $outputs should be an array of TypeInfo objects giving the types of
         * the given outputs.
         */
        public function apply( array $inputs, array $outputs ) {
            // Ensure that the outputs we are given are exactly the same as the
            // ones we expect.

            $this->CheckFuzzy( $this->input, $inputs, 'input' );
            $this->CheckExact( $this->output, $outputs, 'output' );

            // If we've gotten this far, then everything is compatible.
            // Return ourselves as the information.
            return $this;
        }

        public function lookup() {
            $inputs = [];
            foreach( $this->input as $name => $type ) {
                $inputs[$name] = is_type($type) ? $type->lookup() : $type;
            }

            $outputs = [];
            foreach( $this->output as $name => $type ) {
                $outputs[$name] = is_type($type) ? $type->lookup() : $type;
            }

            $sargs = [];
            foreach( $this->req_states() as $name => $type ) {
                $sargs[$name] = $type->lookup();
            }

            return lookupGLA($this->name(), $this->template_args(), $inputs, $outputs, $sargs, null, $this->hash());
        }
    }

    class GF_Info extends GeneralizedObject  {

        private $input = [];

        public function __construct( $hash, $name, $value, array $args, array $oArgs ) {
            parent::__construct(InfoKind::T_GF, $hash, $name, $value, $args, $oArgs[0]);

            grokit_assert( array_key_exists( 'input', $args ),
                'No inputs declared for ' . $this );

            $this->input = $args['input'];
        }

        public function summary() {
            $ret = parent::summary();

            $ret['input'] = squash($this->input);

            return $ret;
        }

        // Getters
        public function input() { return $this->input; }

        /*
         * $outputs should be an array of TypeInfo objects giving the types of
         * the given outputs.
         */
        public function apply( array $inputs, array $states ) {
            // Ensure that the outputs we are given are exactly the same as the
            // ones we expect.

            $this->CheckFuzzy( $this->input, $inputs, 'input' );

            // If we've gotten this far, then everything is compatible.
            // Return ourselves as the information.
            return $this;
        }
    }

    class GIST_Info extends GeneralizedObject  {

        private $output = [];
        private $result_type;
        private $iterable = false;
        private $intermediates = false;
        private $finalize_as_state = false;
        private $chunk_boundary = false;

        public function __construct( $hash, $name, $value, array $args, array $oArgs ) {
            parent::__construct(InfoKind::T_GIST, $hash, $name, $value, $args, $oArgs[0]);

            grokit_assert( array_key_exists( 'output', $args ),
                'No outputs declared for ' . $this );

            $this->output = $args['output'];

            grokit_assert( array_key_exists( 'result_type', $args ),
                'No result type declared for ' . $this );

            $this->result_type = $args['result_type'];
            if( ! is_array($this->result_type) ) {
                $this->result_type = [ $this->result_type ];
            }
            if( array_key_exists( 'iterable', $args ) ) {
                $this->iterable = $args['iterable'];
            }

            if( array_key_exists( 'intermediates' , $args ) ) {
                $this->intermediates = $args['intermediates'];
            }

            if( array_key_exists( 'finalize_as_state', $args ) ) {
                $this->finalize_as_state = $args['finalize_as_state'];
            }

            if( array_key_exists( 'chunk_boundary', $args ) ) {
                $this->chunk_boundary = $args['chunk_boundary'];
            }
        }

        public function summary() {
            $ret = parent::summary();

            $ret['output'] = squash($this->output);
            $ret['result_type'] = $this->result_type;
            $ret['iterable'] = $this->iterable;
            $ret['intermediates'] = $this->intermediates;
            $ret['finalize_as_state'] = $this->finalize_as_state;
            $ret['chunk_boundary'] = $this->chunk_boundary;

            return $ret;
        }

        public function lookup() {
            $outputs = [];
            foreach( $this->output as $name => $type ) {
                $outputs[$name] = is_type($type) ? $type->lookup() : $type;
            }

            $sargs = [];
            foreach( $this->req_states() as $name => $type ) {
                $sargs[$name] = $type->lookup();
            }

            return lookupGIST($this->name(), $this->template_args(), $outputs, $sargs, null, $this->hash());
        }

        // Getters
        public function output() { return $this->output; }
        public function outputs() { return $this->output; }
        public function result_type() { return $this->result_type; }
        public function iterable() { return $this->iterable; }
        public function intermediates() { return $this->intermediates; }
        public function finalize_as_state() { return $this->finalize_as_state; }
        public function chunk_boundary() { return $this->chunk_boundary; }

        /*
         * $outputs should be an array of TypeInfo objects giving the types of
         * the given outputs.
         */
        public function apply( array $outputs ) {
            // Ensure that the outputs we are given are exactly the same as the
            // ones we expect.

            $this->CheckExact( $this->output, $outputs, 'output' );

            // If we've gotten this far, then everything is compatible.
            // Return ourselves as the information.
            return $this;
        }
    }

    class GT_Info extends GeneralizedObject {

        private $output = [];
        private $input = [];
        private $result_type;
        private $chunk_boundary = false;
        private $iterable = false;


        public function __construct( $hash, $name, $value, array $args, array $oArgs ) {
            parent::__construct(InfoKind::T_GT, $hash, $name, $value, $args, $oArgs[0]);

            grokit_assert( array_key_exists( 'output', $args ),
                'No outputs declared for ' . $this );

            $this->output = $args['output'];

            grokit_assert( array_key_exists( 'input', $args ),
                'No inputs declared for ' . $this );

            $this->input = $args['input'];

            grokit_assert( array_key_exists( 'result_type', $args ),
                'No result type declared for ' . $this );

            $this->result_type = $args['result_type'];

            if( array_key_exists( 'chunk_boundary', $args ) ) {
                $this->chunk_boundary = $args['chunk_boundary'];
            }

            if( array_key_exists( 'iterable', $args ) ) {
                $this->iterable = $args['iterable'];
            }
        }

        public function summary() {
            $ret = parent::summary();

            $ret['input'] = squash($this->input);
            $ret['output'] = squash($this->output);
            $ret['result_type'] = $this->result_type;
            $ret['chunk_boundary'] = $this->chunk_boundary;
            $ret['iterable'] = $this->iterable;

            return $ret;
        }

        // Getters
        public function output() { return $this->output; }
        public function input() { return $this->input; }
        public function result_type() { return $this->result_type; }
        public function chunk_boundary() { return $this->chunk_boundary; }
        public function iterable() { return $this->iterable; }

        /*
         * $outputs should be an array of TypeInfo objects giving the types of
         * the given outputs.
         */
        public function apply( array $inputs, array $outputs ) {
            // Ensure that the outputs we are given are exactly the same as the
            // ones we expect.

            $this->CheckFuzzy( $this->input, $inputs, 'input' );
            $this->CheckExact( $this->output, $outputs, 'output' );

            // If we've gotten this far, then everything is compatible.
            // Return ourselves as the information.
            return $this;
        }
    }

    class GSE_Info extends GeneralizedObject {
        public function __construct( $hash, $name, $value, array $args, array $oArgs ) {
            parent::__construct(InfoKind::T_GSE, $hash, $name, $value, $args, $oArgs[0]);
        }

        public function summary() {
            $ret = parent::summary();

            return $ret;
        }

        public function apply() {
            return $this;
        }
    }
}

?>
