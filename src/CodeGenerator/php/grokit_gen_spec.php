<?php

// Copyright 2013 Tera Insights, LLC. All Rights Reserved.

/*
 * This file contains the definitions of classes that represent possibly
 * incomplete specifications of GLAs, GFs, GTs, GISTS, and GIs.
 *
 * As these types may require knowledge of their inputs and/or outputs to
 * instantiate themselves, which may not be known at the time their specification
 * is processed, this information is stored in one of the following classes
 * to be completed later when that information is known.
 */

namespace grokit {

    /*
     * This is the base class for all of the other specification classes.
     */
    abstract class GenSpec extends InfoKind {
        protected $name;
        protected $t_args;
        protected $alias;
        protected $source;

        public function __construct( $kind, $source, $name, array $t_args = null, $alias = null ) {
            parent::__construct($kind);
            $this->name = $name;
            $this->t_args = is_null($t_args) ? [] : $t_args;
            $this->alias = $alias;
            $this->source = $source;
        }

        public function name() { return $this->name; }

        public function hash() {
            $hasher = hash_init( 'sha256' );

            hash_update( $hasher, $this->kind() );
            hash_update( $hasher, $this->name );
            hash_update( $hasher, hashComplex($this->t_args) );

            return hash_final($hasher);
        }

        public function __toString() {
            $ret = "" . $this->name;
            $ret .= "<";
            $ret .= squashToString($this->t_args, '<', '>');
            $ret .= ">";

            return $ret;
        }
    }

    class GI_Spec extends GenSpec {
        public function __construct( $source, $name, array $t_args = null, $alias = null) {
            parent::__construct( InfoKind::T_GI, $source, $name, $t_args, $alias);
        }

        public function __toString() {
            return "GI:" . parent::__toString();
        }

        // Perform the lookup for the GI.
        // If given, $outputs should be an array of TypeInfos giving the outputs
        // expected by the waypoint.
        public function apply( $outputs = [] ) {
            try {
                return lookupGI( $this->name, $this->t_args, $outputs, $this->alias );
            }
            catch( Exception $e ) {
                grokit_error( 'Failed to lookup GI ' . $this->name . ' from spec ' . $this->source, $e);
            }
        }
    }

    class GLA_Spec extends GenSpec {
        public function __construct( $source, $name, array $t_args = null, $alias = null) {
            parent::__construct( InfoKind::T_GLA, $source, $name, $t_args, $alias);
        }

        public function __toString() {
            return "GLA:" . parent::__toString();
        }

        // Perform the lookup for the GLA.
        // If given, $outputs should be an array of TypeInfos giving the outputs
        // expected by the waypoint.
        public function apply( $inputs, $outputs, $sargs = [] ) {
            try {
                $input = [];
                foreach( $inputs as $n => $v ) {
                    if( is_datatype( $v ) ) {
                        $input[$n] = $v;
                    }
                    else {
                        $input[$n] = $v->type();
                    }
                }
                return lookupGLA( $this->name, $this->t_args, $input, $outputs, $sargs, $this->alias );
            }
            catch( Exception $e ) {
                grokit_error( 'Failed to lookup GLA ' . $this->name . ' from spec ' . $this->source, $e);
            }
        }
    }

    class GF_Spec extends GenSpec {
        public function __construct( $source, $name, array $t_args = null, $alias = null) {
            parent::__construct( InfoKind::T_GF, $source, $name, $t_args, $alias);
        }

        public function __toString() {
            return "GF:" . parent::__toString();
        }

        // Perform the lookup for the GF.
        // If given, $outputs should be an array of TypeInfos giving the outputs
        // expected by the waypoint.
        public function apply( $inputs ) {
            try {
                $input = [];
                foreach( $inputs as $n => $v ) {
                    $input[$n] = $v->type();
                }
                return lookupGF( $this->name, $this->t_args, $input, $this->alias );
            }
            catch( Exception $e ) {
                grokit_error( 'Failed to lookup GF ' . $this->name . ' from spec ' . $this->source, $e);
            }
        }
    }

    class GIST_Spec extends GenSpec {
        public function __construct( $source, $name, array $t_args = null, $alias = null) {
            parent::__construct( InfoKind::T_GIST, $source, $name, $t_args, $alias);
        }

        public function __toString() {
            return "GIST:" . parent::__toString();
        }

        // Perform the lookup for the GIST.
        // If given, $outputs should be an array of TypeInfos giving the outputs
        // expected by the waypoint.
        public function apply( $outputs, $sargs = [] ) {
            try {
                return lookupGIST( $this->name, $this->t_args, $outputs, $sargs, $this->alias );
            }
            catch( Exception $e ) {
                grokit_error( 'Failed to lookup GIST ' . $this->name . ' from spec ' . $this->source, $e);
            }
        }
    }

    class GT_Spec extends GenSpec {
        public function __construct( $source, $name, array $t_args = null, $alias = null) {
            parent::__construct( InfoKind::T_GT, $source, $name, $t_args, $alias);
        }

        public function __toString() {
            return "GT:" . parent::__toString();
        }

        // Perform the lookup for the GT.
        // If given, $outputs should be an array of TypeInfos giving the outputs
        // expected by the waypoint.
        public function apply( $inputs, $outputs, $sargs = [] ) {
            try {
                $input = [];
                foreach( $inputs as $n => $v ) {
                    if( is_datatype($v) ) {
                        $input[$n] = $v;
                    } else {
                        $input[$n] = $v->type();
                    }
                }
                return lookupGT( $this->name, $this->t_args, $input, $outputs, $sargs, $this->alias );
            }
            catch( Exception $e ) {
                grokit_error( 'Failed to lookup GT ' . $this->name . ' from spec ' . $this->source, $e);
            }
        }
    }

}
?>
