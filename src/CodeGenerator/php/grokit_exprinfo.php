<?php

// Copyright 2013 Tera Insights, LLC. All Rights Reserved.

namespace grokit {

    require_once 'grokit_base.php';

    // Represents an expression
    class ExpressionInfo {
        // String representing the source the expression came from.
        // Useful for debugging and error messages
        private $source = '';

        // Type of the expression
        private $type = 'NO TYPE';

        // The value of the expression, ready to be dumped into C++
        private $value = '';

        // Whether or not this expression is constant
        private $is_const = false;

        // Any constants that need to be defined before any tuples are processed by
        // this expression.
        private $constants = [];

        // Array of statements that need to be run before each tuple.
        private $preprocess = [];

        // A list of libraries required by types and subexpressions
        private $libraries = [];

        public function __construct( $source, $type, $value, $is_const ) {
            $this->source = $source;
            $this->type = $type;
            $this->value = $value;
            $this->is_const = $is_const;

            if( $type !== null )
                $this->libraries = $type->libraries();
        }

        public function __toString() {
            return $this->value;
        }

        public function type() { return $this->type; }
        public function value() { return $this->value; }
        public function is_const() { return $this->is_const; }
        public function constants() { return $this->constants; }
        public function preprocess() { return $this->preprocess; }
        public function source() { return $this->source; }
        public function libraries() { return $this->libraries; }

        public function setType( $type ) {
            grokit_logic_assert( $this->type === null,
                'Setting type of expression with an already set type.');

            $this->type = $type;
            $this->libraries = array_unique(array_merge($this->libraries, $type->libraries()));
        }

        // Absorb constants and preprocessing information from other
        // ExpressionInfos
        public function absorbMeta( ExpressionInfo &$info ) {
            $this->constants = array_merge($this->constants, $info->constants);
            $this->preprocess = array_merge($this->preprocess, $info->preprocess);
            $this->is_const = $this->is_const && $info->is_const;
            $this->libraries = array_unique(array_merge($this->libraries, $info->libraries()));
        }

        // Functions to add metadata

        public function addConstant( $const ) {
            $this->constants[] = $const;
        }

        public function addConstants( array $consts ) {
            $this->constants = array_merge( $this->constants, $consts );
        }

        public function addPreprocess( $pre ) {
            $this->preprocess[] = $pre;
        }

        public function addPreprocesses( array $pres ) {
            $this->preprocess = array_merge( $this->preprocess, $pres );
        }

        public function addLibraries( array $libs ) {
            $this->libraries = array_unique(array_merge($this->libraries, $libs));
        }

        // Generate a new constant from this expression.
        // This replaces the value with an automatically generated variable,
        // which is defined by a statement that is added to the list of constants.
        public function makeConstant() {
            grokit_logic_assert($this->is_const,
                'Attempting to add non-constant value (' . $this->value . ') to constants');

            $newName = generate_name('ct_');
            $ctVal = 'const ' . $this->type . ' ' . $newName . ' = ' . $this->value . ';';

            // Add all preprocessing statements to the list of constants.
            $this->constants = array_merge($this->constants, $this->preprocess);
            $this->preprocess = [];

            // Add the constant statement to the list and then set the value to the
            // attribute name.
            $this->constants[] = $ctVal;
            $this->value = $newName;
        }
    }

}

namespace {
    function is_expression( $val ) {
        return is_object($val) && is_a($val, 'grokit\\ExpressionInfo');
    }
}

?>
