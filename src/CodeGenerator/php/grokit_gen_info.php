<?php

// Copyright 2013 Tera Insights, LLC. All Rights Reserved.

/*
 * This file contains a class that keeps track of information produced by
 * waypoints.
 */

namespace grokit {
    class GenerationInfo {
        // Files per waypoint
        private $files = [];

        // Jobs per waypoint
        private $jobs = [];

        // Libraries needed
        private $libs = [];

        public function __construct( ) {
        }

        public function absorb( GenerationInfo & $other ) {
            $this->addFiles($other->files);
            $this->addLibs($other->libs);
            $this->addJobs($other->jobs);
        }

        // Getters
        public function files() {
            $ret = [];
            foreach( $this->files as $wp => $fileList ) {
                $ret = array_merge($ret, $fileList);
            }
            return array_unique($ret);
        }

        public function libs() {
            return $this->libs;
        }

        public function filesPerWaypoint() {
            return $this->files;
        }

        public function jobsPerWaypoint() {
            return $this->jobs;
        }

        public function filesPerJob() {
            $ret = [];
            foreach( $this->files as $wp => $fileList ) {
                if( array_key_exists($wp, $this->jobs) )
                    $jobs = $this->jobs[$wp];
                else
                    $jobs = $this->jobs();

                foreach( $jobs as $job ) {
                    if( !array_key_exists($job, $ret) )
                        $ret[$job] = [];

                    $ret[$job] = array_unique(array_merge($ret[$job], $fileList));
                }
            }

            return $ret;
        }

        public function jobs() {
            $ret = [];
            foreach( $this->jobs as $wp => $jobs ) {
                $ret = array_merge($ret, $jobs);
            }

            return array_unique($ret);
        }

        // Manipulators

        public function addLibs( array $libs ) {
            $this->libs = array_unique(array_merge($this->libs, $libs));
        }

        public function addFile( $file, $wp ) {
            $this->files[$wp][] = $file;
        }

        public function addJob( $job, $wp ) {
            $this->jobs[$wp][] = $job;
        }

        public function addFiles( array $files ) {
            foreach( $files as $wp => $oFiles ) {
                if( array_key_exists($wp, $this->files) ) {
                    $this->files[$wp] = array_merge($this->files[$wp], $oFiles);
                }
                else {
                    $this->files[$wp] = $oFiles;
                }
            }
        }

        public function addJobs( array $jobs ) {
            foreach( $jobs as $wp => $ojobs ) {
                if( array_key_exists($wp, $this->jobs) ) {
                    $this->jobs[$wp] = array_merge($this->jobs[$wp], $ojobs);
                }
                else {
                    $this->jobs[$wp] = $ojobs;
                }
            }
        }


        // Absorb functions for various types
        public function absorbInfo( $info ) {
            grokit_assert( is_object($info), "Called absorbInfo on non-object: " . print_r($info, true) );
            $this->addLibs($info->libraries());
        }

        public function absorbInfoList( array $list ) {
            foreach( $list as $elem ) {
                $this->absorbInfo($elem);
            }
        }

        public function absorbAttr( $attr ) {
            if( $attr->type() == null ) {
                grokit_error("Attribute has no type set: " . print_r($attr, true));
            }
            $this->absorbInfo( $attr->type() );
        }

        public function absorbAttrList( array $list ) {
            try {
                foreach( $list as $elem ) {
                    $this->absorbAttr($elem);
                }
            } catch( Exception $e) {
                grokit_logic_error("Failed to absorb info for attribute list: " . print_r($list, true), $e);
            }
        }

        public function absorbStateList( array $list ) {
            foreach( $list as $elem ) {
                $this->absorbInfo( $elem->type() );
            }
        }
    }
}
