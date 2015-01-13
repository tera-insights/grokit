<?php

// Copyright 2013 Tera Insights, LLC. All Rights Reserved.

/*
 * This file contains a class used to represent functors.
 */

namespace grokit {

    class Functor {
        // The source of the functor
        private $source;

        // The name of the functor
        private $name;

        // The arguments of the functor
        private $args;

        // Hash that represents the functor
        // Initially null, lazily evaluated.
        private $hash = null;

        public function __construct( $source, $name, $args ) {
            $this->source = $source;
            $this->name = $name;
            $this->args = $args;
        }

        // Getters
        public function source() { return $this->source; }
        public function name() { return $this->name; }
        public function args() { return $this->args; }

        public function hash() {
            if( $this->hash === null ) {
                $hasher = hash_init( 'sha256' );

                hash_update( $hasher, $this->name );

                hash_update( $hasher, '(' );
                hash_update( $hasher, hashComplex($this->args));
                hash_update( $hasher, ')' );

                $this->hash = hash_final($hasher);
            }

            return $this->hash;
        }

        public function __toString() {
            return $this->name . "(" . implode(", ", array_map('strval', $this->args)) . ")";
        }
    }

}

?>
