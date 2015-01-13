<?php

// Copyright 2013 Tera Insights, LLC. All Rights Reserved.

/*
 * This file contains the definition of the namespace class, which is used
 * to keep track of types, aliases, etc.
 */

namespace grokit {

    require_once 'grokit_typeinfo.php';

    class LibNamespace {
        /***** Class Constants *****/

        // Namespace separators
        const NS_SEP     = '::';
        const PHP_NS_SEP = '\\';

        // Constants for kinds
        const K_TYPE = 'type';
        const K_GLA  = 'gla';
        const K_GF   = 'gf';
        const K_GT   = 'gt';
        const K_GI   = 'gi';
        const K_GIST = 'gist';

        /***** Class Members *****/

        private $name;      // The name of the namespace
        private $parent;    // The parent of this namespace
        // The fully qualified name of a namespace is the name of the
        // parent (if any) joined with its own name.

        // Mappings for aliases
        // All alias mappings are from a type local to this namespace
        // to a fully qualified type name.
        // This is a two-dimensional array. First level is the kind of thing
        // being aliased, second level is the name of the alias.
        private $aliasMap = [];

        // Inner namespaces
        private $innerNS = [];

        // Cache for type information. This can be easily done, as types are
        // completely defined by their name and template arguments.
        private $typeCache = [];

        /***** Constructor *****/
        // Set the namespace's name and optionally its parent
        public function __construct( $name, &$parent = null ) {
            $this->name = $name;
            $this->parent = &$parent;
        }

        // Function to set the parent to a new namespace.
        // This is used when cloning namespaces and moving them around.
        private function setParent( &$parent ) {
            $this->parent = &$parent;
        }

        // Clone function so that we can easily do using and save the state
        // of the library.
        public function __clone( ) {
            // For each inner namespace, change the parent to me.
            foreach( $innerNS as $ns ) {
                $ns->setParent( $this );
            }
        }

        /***** Methods *****/

        // Methods to get the name of the namespace
        public function fullName() {
            if( $parent != null ) {
                return self::JoinNamespace($parent->fullName(), $this->name);
            }
            else {
                return $this->name;
            }
        }

        public function localName() {
            return $this->name;
        }

        public function name() {
            return $this->fullName();
        }

        // Returns the name as a PHP namespace
        public function phpName() {
            if( $parent != null ) {
                return self::JoinNamespacePHP($parent->phpName(), $this->name);
            }
            else {
                return PHP_NS_SEP . $this->name;
            }
        }

        // Methods to get inner namespaces and automatically construct them
        // if they do not exist.
        private function &getInnerNS( $name ) {
            if( ! array_key_exists($name, $this->innerNS) ) {
                $this->innerNS[$name] = new LibNamespace($name, $this);
            }
            return $this->innerNS[$name];
        }

        // Aliasing functions
        public function createAlias( $kind, $from, $to ) {
            if( self::IsNamespaced( $from ) ) {
                // Need to add the alias to an inner namespace.
                $parts = self::SplitNamespace($from);
                $ns = $parts[0];
                $name = $parts[1];

                $inner = & getInnerNS( $ns );

                $inner->createAlias( $kind, $name, $to );
            }
            else {
                if( ! array_key_exists($kind, $this->aliasMap) ) {
                    $this->aliasMap[$kind] = [];
                }

                grokit_assert( ! array_key_exists($from, $this->aliasMap[$kind]),
                    'Attempting to insert duplicate alias for type ' . $from .
                    ' in namespace ' . $this->fullName() );

                $this->aliasMap[$kind][$from] = $to;
            }
        }

        public function normalizeName( $kind, $alias ) {
            if( self::IsNamespaced( $name ) ) {
                // look up the alias in an inner namespace
                $parts = self::SplitNamespace($alias);
                $ns = $parts[0];
                $name = $parts[1];

                $inner = & getInnerNS( $ns );

                return $this->localName() . $inner->normalizeName( $kind, $name );
            }
            else {
                if( ! array_key_exists( $kind, $this->aliasMap ) ) {
                    // We don't have ANY aliases for this type, so this has
                    // to be a base name.
                    return $alias;
                }

                if( array_key_exists( $alias, $this->aliasMap[$kind] ) ) {
                    // We found an alias mapping.
                    return $this->aliasMap[$kind][$alias];
                }
                else {
                    // No alias found, must be a base name already
                    return $alias;
                }
            }
        }

        // Look up a type
        // The name given MUST have been normalized already or the lookup
        // will fail.
        public function lookupType( $name, &$t_args ) {
            if( self::IsNamespaced($name) ) {
                // Look up the type in the inner namespace
                $parts = self::SplitNamespace($alias);
                $ns = $parts[0];
                $base = $parts[1];

                $inner = & getInnerNS($ns);

                return $inner->lookupType( $base, $t_args );
            }
            else {
                // See if we have the type in the cache.
                // For now, we will only do this for basic types
                if( $t_args === null ) {
                    if( array_key_exists( $name, $this->typeCache ) ) {
                        return $this->typeCache[$name];
                    }
                }

                $fullName = self::JoinNamespace( $this->fullName(), $name );

                // It wasn't in the cache, so see if the function exists
                // to instantiate it.
                $php_ns = self::JoinNamespacePHP( $this->phpName(), 'Type');
                $cpp_ns = self::JoinNamespace( $this->fullName(), 'Type' );
                $func_name = self::JoinNamespacePHP( $php_ns, $name );

                grokit_assert( function_exists($func_name),
                    'Unable to instantiate type ' . $fullName . ': No corresponding ' .
                'function ' . $func_name . ' found' );

                $args = [];
                if( $t_args !== null ) {
                    $args[] = $t_args;
                }
                $info = self::CallGenerator( $cpp_ns, $func, $args );

                // Create a TypeInfo for the type.
                $tName = self::JoinNamespace( $cpp_ns, $info['name'] );

                $dt = new DataType( $fullName, $tName, $info );

                // Store the type in the cache
                // Only for basic types right now
                $this->typeCache[$name] = $dt;

                return $dt;
            }
        }

        /***** Static Methods *****/

        // Joins the names of namespaces properly.
        static public function JoinNamespace( $top, $bottom ) {
            if( $top != '' ) {
                return $top . NS_SEP . $bottom;
            }
            else {
                return $bottom;
            }
        }

        static public function JoinNamespacePHP( $top, $bottom ) {
            $top = rtrim($top, PHP_NS_SEP);
            $bottom = ltrim($bottom, PHP_NS_SEP);
            return $top . PHP_NS_SEP . $bottom;
        }

        // Determines whether or not a name is namespaced.
        static public function IsNamespaced( $name ) {
            return strpos($name, NS_SEP) !== false;
        }

        // Splits a name at the first namespace.
        static public function SplitNamespace( $name ) {
            grokit_logic_assert( self::IsNamespaced( $name ),
                'Attempting to split non-namespaced name ' . $name .
                ' by namespace.');

            return $parts = explode(NS_SEP, $name, 2);
        }

        // Calls a generation function, and wraps the generated code in its
        // namespace
        static public function CallGenerator( $ns, $func, &$args ) {
            $ns_parts = explode(NS_SEP, $ns);

            $preamble = '';
            $postamble = '';

            foreach( $ns_parts as $part ) {
                $preamble .= 'namespace ' . $part . '{' . PHP_EOL;
                $postamble .= PHP_EOL . '} // end namespace ' . $part . PHP_EOL;
            }

            echo $preamble;
            $res = call_user_func_array( $func, $args );
            echo $postabmel;

            return $res;
        }
    }
}

?>
