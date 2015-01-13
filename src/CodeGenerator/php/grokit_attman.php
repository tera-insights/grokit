<?php

// Copyright 2013 Tera Insights, LLC. All Rights Reserved.

/*
 * This file contains utilities to keep track of Attributes in the system and
 * query them for information about them (type, slot, etc.)
 */

namespace grokit {

    class AttributeInfo {
        // The name of the attribute
        private $name;

        // This is the slot number of the attribute.
        private $slot;

        // The AST node for the type.
        private $type_ast;

        private $type = null;

        public function __construct( $name, $type_ast, $slot ) {
            $this->name = $name;
            $this->type_ast = $type_ast;
            $this->slot = $slot;
        }

        public function __toString() {
            return $this->name;
        }

        public function name() {
            return $this->name;
        }

        public function type() {
            if( !is_null($this->type) ) {
                return lookupType($this->type->name(), $this->type->template_args());
            }

            if( is_null($this->type_ast) )
                return null;

            return parseType($this->type_ast);
        }

        public function setType( $type ) {
            $this->type = $type;
        }

        public function slot() {
            return $this->slot;
        }

        // Force the evaluation of the type.
        public function evalType() {
            parseType($this->type_ast);
        }
    }

    class AttributeManager {
        // Mapping of attribute name to information
        private static $att_map = [];

        public static function addAttribute( $name, $type, $slot ) {
            grokit_logic_error_if(isset(self::$att_map[$name]),
                'Attempting to add attribute ' . $name . ' twice');

            $info = new AttributeInfo($name, $type, $slot);
            self::$att_map[$name] = $info;
        }

        public static function lookupAttribute( $name ) {
            if( !array_key_exists($name, self::$att_map) ) {
                fwrite(STDERR, 'lookupAttribute called with name: ' . print_r($name, true) );
                //fwrite(STDERR, print_r(self::$att_map, true) );
            }

            grokit_assert( array_key_exists($name, self::$att_map),
                'Attempting to lookup unknown attribute ' . $name);

            return self::$att_map[$name];
        }

        public static function attributeExists( $name ) {
            return isset(self::$att_map[$name]);
        }

        public static function setAttributeType( $name, $type_ast ) {
            grokit_logic_assert( self::attributeExists($name),
                'No attribute named ' . $name . ' exists');

            //fwrite(STDERR, "Setting type of attribute $name to " . print_r($type_ast, true) . PHP_EOL);

            self::$att_map[$name]->setType($type_ast);
        }

        public static function debugPrint() {
            fwrite(STDERR, "AttributeManager contents:\n");
            fwrite(STDERR, print_r(self::$att_map, true));
        }
    }

}
namespace {
    // Dispatching function so we can change the implementation later without
    // having to hunt through various files to find all references to
    // AttributeManager.
    function lookupAttribute( $name ) {
        return \grokit\AttributeManager::lookupAttribute($name);
    }

    function addAttribute( $name, $type, $slot ) {
        \grokit\AttributeManager::addAttribute($name, $type, $slot);
    }

    function is_attribute( $obj ) {
        return is_object($obj) && is_a($obj, 'grokit\AttributeInfo');
    }
}

?>
