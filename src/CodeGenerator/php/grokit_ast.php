<?php
namespace grokit {

    // Copyright 2013 Tera Insights, LLC. All Rights Reserved.

    /*
     * this file contains helper functions for interacting with the AST.
     */

    function ast_has( $ast, $key ) {
        if( is_object( $ast ) ) {
            return property_exists($ast, $key);
        }
        else if( is_array( $ast ) ) {
            return array_key_exists($key, $ast);
        }
        else {
            grokit_logic_error( 'Called ' . __FUNCTION__ . ' on variable of type ' . gettype($ast)
            . ', expected array or object' );
        }
    }

    function ast_assert_has( $ast, $key ) {
        grokit_logic_assert( ast_has( $ast, $key ),
            'Node does not have attribute with key ' . $key);
    }

    function ast_get( $ast, $key ) {
        ast_assert_has( $ast, $key );

        if( is_object( $ast ) )
            return $ast->$key;
        else
            return $ast[$key];
    }

    /*
     * This class represents the source information of an AST, and has its
     * __toString method defined so it can be used as a string.
     */
    class ASTSource {
        // Information about the source
        private $file;
        private $line;
        private $col;

        /*
         * This method takes the source information attribute of a node and
         * extracts the information from it.
         */
        public function __construct( &$src_info ) {
            $this->file = ast_get( $src_info, NodeKey::FILENAME );
            $this->line = ast_get( $src_info, NodeKey::LINE );
            $this->col = ast_get( $src_info, NodeKey::COL );
        }

        public function __toString( ) {
            return "[{$this->file}:{$this->line}:{$this->col}]";
        }

        // Getters
        public function file() { return $this->file; }
        public function line() { return $this->line; }
        public function col() { return $this->col; }
    }

    function ast_node_source( $ast ) {
        grokit_logic_assert( ast_has($ast, NodeKey::NODE_SOURCE),
            'Node does not have a source!');

        $source = ast_get( $ast, NodeKey::NODE_SOURCE );

        return new ASTSource( $source );
    }

    function ast_node_type( $ast ) {
        grokit_logic_assert( ast_has($ast, NodeKey::NODE_TYPE),
            'Node does not have a type!');

        return ast_get( $ast, NodeKey::NODE_TYPE );
    }

    // Assert that an AST node is of a particular type.
    function assert_ast_type( $ast, $expected ) {
        $type = ast_node_type($ast);
        if( is_string( $expected ) ) {
            grokit_logic_assert( $type == $expected,
                'Node Type Mismatch: Exptected ' . $expected . ', got '
                . $type . ' from ' . ast_node_source($ast) );
        }
        else if( is_array( $expected ) ) {
            grokit_logic_assert( in_array( $type, $expected ),
                'Node Type Mismatch: Expected one of [' . implode(', ', $expected)
                . '] got ' . $type . ' from ' . ast_node_source($ast));
        }
    }

    function ast_node_data( $ast ) {
        grokit_logic_assert( ast_has($ast, NodeKey::NODE_DATA),
            'Attempted to get data of non-node value.');

        return ast_get( $ast, NodeKey::NODE_DATA );
    }

    function make_ast_node( $type, $source, $data ) {
        return [ NodeKey::NODE_SOURCE => $source, NodeKey::NODE_TYPE => $type, NodeKey::NODE_DATA => $data ];
    }
}

?>
