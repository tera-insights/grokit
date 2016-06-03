<?php

// Copyright 2013 Tera Insights, LLC. All Rights Reserved.

/*
 * This file contains a class used to represent template arguments.
 */

namespace {
    function is_datatype( $object ) {
        return is_a($object, 'grokit\\DataType', false);
    }

    function is_functor($object) {
        return is_a($object, 'grokit\\Functor', false);
    }

    function is_identifier($object) {
        return is_a($object, 'grokit\\Identifier', false);
    }

    function is_gla($object) {
        return is_a($object, 'grokit\\GLA_Info', false) || is_a($object, 'grokit\\GLA_Spec');
    }

    function is_gf($object) {
        return is_a($object, 'grokit\\GF_Info', false) || is_a($object, 'grokit\\GF_Spec');
    }

    function is_gt($object) {
        return is_a($object, 'grokit\\GT_Info', false) || is_a($object, 'grokit\\GT_Spec');
    }

    function is_gist($object) {
        return is_a($object, 'grokit\\GIST_Info', false) || is_a($object, 'grokit\\GIST_Spec');
    }

    function is_gi($object) {
        return is_a($object, 'grokit\\GI_Info', false) || is_a($object, 'grokit\\GI_Spec');
    }

    function is_type( $object ) {
        return is_datatype($object) || is_gla($object);
    }

    function is_typeinfo( $object ) {
        return is_a($object, 'grokit\\TypeInfo');
    }

    function is_resource( $object ) {
        return is_a($object, 'grokit\\Resource_Info');
    }
}


namespace grokit {

    require_once 'grokit_base.php';

    // Small class to wrap identifiers so we know it's an identifer and not a
    // string
    class Identifier {
        private $val;

        public function __construct( $val ) {
            $this->val = $val;
        }

        public function __toString() {
            return $this->val;
        }

        public function value() {
            return $this->val;
        }

        public function hash() {
            $hasher = hash_init( 'sha256' );

            hash_update( $hasher, 'identifier' );
            hash_update( $hasher, $this->val );

            return hash_final($hasher);
        }
    }

    function hashComplex( $val ) {
        $hasher = hash_init( 'sha256' );

        if( is_array($val) ) {
            hash_update( $hasher, '[' );
            // Ensure array is sorted by keys
            ksort($val);

            foreach( $val as $name => $v ) {
                hash_update( $hasher, $name );
                hash_update( $hasher, '=>' );
                hash_update( $hasher, hashComplex($v) );
            }

            hash_update( $hasher, ']' );
        }
        else if( is_gla($val) ) {
            hash_update( $hasher, 'gla' );
            hash_update( $hasher, $val->hash() );
        }
        else if( is_gf($val) ) {
            hash_update( $hasher, 'gf' );
            hash_update( $hasher, $val->hash() );
        }
        else if( is_gt($val) ) {
            hash_update( $hasher, 'gt' );
            hash_update( $hasher, $val->hash() );
        }
        else if( is_gist($val) ) {
            hash_update( $hasher, 'gist' );
            hash_update( $hasher, $val->hash() );
        }
        else if( is_gi($val) ) {
            hash_update( $hasher, 'gi' );
            hash_update( $hasher, $val->hash() );
        }
        else if( is_datatype($val) ) {
            hash_update( $hasher, 'datatype' );
            hash_update( $hasher, $val->hash() );
        }
        else if( is_resource($val) ) {
            hash_update( $hasher, 'resource' );
            hash_update( $hasher, $val->hash() );
        }
        else if( is_functor($val) ) {
            hash_update( $hasher, 'functor' );
            hash_update( $hasher, $val->hash() );
        }
        else if( is_identifier($val) ) {
            hash_update( $hasher, 'identifier' );
            hash_update( $hasher, $val->hash() );
        }
        else if( is_attribute($val) ) {
            hash_update( $hasher, 'attribute' );
            hash_update( $hasher, strval($val) );
        }
        else if( is_int($val) || is_float($val) || is_string($val) || is_bool($val) ) {
            hash_update( $hasher, gettype($val) );
            hash_update( $hasher, $val );
        }
        else if( is_null($val) ) {
            hash_update( $hasher, 'null' );
        }
        else {
            $valType = is_object($val) ? get_class($val) : gettype($val);
            grokit_logic_error('Unable to hash unknown type ' . $valType );
        }

        return hash_final($hasher);
    }

}

?>
