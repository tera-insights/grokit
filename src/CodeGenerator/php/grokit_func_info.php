<?php

// Copyright 2013 Tera Insights, LLC. All Rights Reserved.

/*
 * This file contains the definitions of classes that represent functions
 * and operators in the system.
 */

namespace grokit {

    class OperatorInfo extends TypeInfo {
        use ArgumentChecker;
        /***** Class Members and Functions *****/

        // List of arguments ( type information ) to the operator.
        private $args;

        // The result type of the function.
        private $resultType;

        // Whether or not the function is deterministic
        // Default for operators is true
        private $deterministic = false;

        public function __construct( $hash, $name, $value, array &$args ) {
            parent::__construct( InfoKind::T_OP, $hash, $name, $value, $args, [] );

            //fwrite(STDERR, 'OperatorInfo args: ' . print_r($args, true) . PHP_EOL );

            grokit_assert( array_key_exists( 'input', $args ),
                'Malformed return value from function generator ' . $name .
                ': No input defined.' );

            $this->args = $args['input'];

            grokit_assert( array_key_exists( 'result', $args ),
                'Malformed return value from function generator ' . $name .
                ': No result type defined.' );

            $this->resultType = $args['result'];

            if( array_key_exists( 'deterministic', $args ) ) {
                $this->deterministic = $args['deterministic'];
            }
        }

        public function summary() {
            $ret = parent::summary();

            $ret['input'] = squash($this->args);
            $ret['output'] = squash($this->resultType);
            $ret['deterministic'] = $this->deterministic;

            return $ret;
        }

        // Applies this operator to a set of expressions.
        // Returns an expression info.
        public function apply( array $exprs, $source ) {
            $this->checkFuzzy( $this->args, $exprs, 'parameter' );

            $exprVals = array_map( function($val){ return $val->value(); }, $exprs );
            $value = $this->value() . '(' . implode(', ', $exprVals) .  ')';

            switch( count($this->args) ) {
            case 1:
                $value = '(' . $this->value() . $exprVals[0] . ')';
                break;
            case 2:
                $value = '(' . $exprVals[0] . ')' . $this->value() . '(' . $exprVals[1] . ')';
                break;
            default:
                grokit_error( 'Got an operator with ' . $count($this->args) . ' arguments!');
            }

            $is_const = $this->deterministic;
            foreach( $exprs as $expr ) {
                $is_const = $is_const && $expr->is_const();
            }

            $info = new ExpressionInfo( $source, $this->resultType, $value, $is_const );

            foreach( $exprs as $expr ) {
                $info->absorbMeta( $expr );
            }

            return $info;
        }
    }

    class FunctionInfo extends TypeInfo {
        use ArgumentChecker;

        // List of arguments ( type information ) to the function.
        private $args;

        // The result type of the function.
        private $resultType;

        // Whether or not the function is deterministic (default is the safe
        // value of false)
        private $deterministic = false;

        public function __construct( $hash, $name, $value, array &$args, array $targs ) {
            $global = get_default($args, 'global', false);
            if( $global ) {
                $nameParts = LibraryManager::SplitNamespace($value);
                $value = $nameParts[\count($nameParts) - 1];
            }
            parent::__construct( InfoKind::T_FUNC, $hash, $name, $value, $args, $targs );

            grokit_assert( array_key_exists( 'input', $args ),
                'Malformed return value from function generator ' . $name .
                ': No input defined.' );

            $this->args = $args['input'];

            grokit_assert( array_key_exists( 'result', $args ),
                'Malformed return value from function generator ' . $name .
                ': No result type defined.' );

            $this->resultType = $args['result'];

            if( array_key_exists( 'deterministic', $args ) ) {
                $this->deterministic = $args['deterministic'];
            }
        }

        public function summary() {
            $ret = parent::summary();

            $ret['input'] = squash($this->args);
            $ret['output'] = squash($this->resultType);
            $ret['deterministic'] = $this->deterministic;

            return $ret;
        }

        // Applies this function to a set of expressions.
        // Returns an expression info.
        public function apply( array $exprs, $source ) {
            $this->checkFuzzy( $this->args, $exprs, 'parameter' );

            $exprVals = array_map( function($val){ return $val->value(); }, $exprs );
            $value = $this->value() . '(' . implode(', ', $exprVals) .  ')';

            $is_const = $this->deterministic;
            foreach( $exprs as $expr ) {
                $is_const = $is_const && $expr->is_const();
            }

            $info = new ExpressionInfo( $source, $this->resultType, $value, $is_const );

            foreach( $exprs as $expr ) {
                $info->absorbMeta( $expr );
            }

            return $info;
        }
    }

    class MethodInfo {
        use ArgumentChecker;

        // Name of the method
        private $name;

        // Types of the arguments
        private $args = [];

        // Result type of the method
        private $resultType;

        // Whether or not the method is deterministic.
        // Deterministic methods should probably not change the object.
        private $deterministic = false;

        public function __construct( $name, array $args, $res, $det = false ) {
            $this->name = $name;
            $this->args = array_values($args);
            $this->resultType = $res;
            $this->deterministic = $det;
        }

        public function summary() {
            return [
                'kind'          => 'METHOD',
                'input'         => squash($this->args),
                'output'        => squash($this->resultType),
                'deterministic' => $this->deterministic,
                ];
        }

        /**
         *  Returns an integer describing the compatibility of the method
         *  with the given types.
         *
         *  @returns -1 if incompatible
         *      0 if perfectly compatible
         *      otherwise the number of implicit conversions required to make compatible
         */
        public function compatibility( array $types ) {
            $types = array_values($types);
            if( count($types) != count($this->args) ) {
                return -1;
            }

            $rating = 0;
            foreach( $this->args as $name => $expected ) {
                $given = $types[$name];

                if( strval($expected) != strval($given) ) {
                    if( CanConvert( $given, $expected ) ) {
                        $rating += 1;
                    }
                    else {
                        return -1;
                    }
                }
            }

            return $rating;
        }

        public function apply( $obj, array $exprs, $source ) {
            $this->checkFuzzy( $this->args, $exprs, 'parameter' );
            $exprVals = array_map( function($val){ return $val->value(); }, $exprs );
            $value = $obj->value() . '.' . $this->name . '(' . implode(', ', $exprVals) . ')';

            $is_const = $this->deterministic && $obj->is_const();
            foreach( $exprs as $expr ) {
                $is_const = $is_const && $expr->is_const();
            }

            $retType = lookupType($this->resultType);
            $info = new ExpressionInfo( $source, $retType, $value, $is_const );
            $info->absorbMeta($obj);
            foreach( $exprs as $expr ) {
                $info->absorbMeta($expr);
            }

            return $info;
        }

        public function __toString() {
            return $this->name;
        }
    }
}

?>
