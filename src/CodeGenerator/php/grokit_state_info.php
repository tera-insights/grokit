<?php

// Copyright 2013 Tera Insights, LLC. All Rights Reserved.

/*
 * This file contains the definition for a class that represents a state required
 * by a GLA/GF/GT/GIST/GI.
 */

namespace grokit {
    class StateInfo {
        // The waypoint that is the source of the state.
        private $waypoint;

        // The type of the state
        private $type;

        // Unique name for the state
        private $name;

        /*
         * $wp should be a string containing the name of the source waypont
         * $type should be one of the G*_Info classes.
         */
        public function __construct( $wp, $type ) {
            $this->waypoint = $wp;
            $this->type = $type;
            $this->name = generate_name($wp . "_state");
        }

        // Getters for attributes
        public function waypoint() { return $this->waypoint; }
        public function type() { return $this->type; }
        public function name() { return $this->name; }
        public function __toString() { return $this->name; }
    }
}

?>
