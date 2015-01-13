<?php
namespace grokit;

// Copyright 2013 Christopher Dudley
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/*
 * This files contains tools to automate the process of writing interface
 * classes that provide Swapping and Copying facilities.
 *
 * The way these macros can be used is the following:
 *
 * grokit\interface_class(ClassName, BaseName, pointerVariable);
 *  grokit\interface_constructor( arguments );
 *  grokit\interface_default_constructor();
 *  grokit\interface_function(name, returnType, arguments );
 * grokit\interface_class_end
 *
 * arguments are an associative array of the form [ name => type, ... ]
 */

require_once('grokit_base.php');

/*
 * Start a new interface class with the given name, parent class,
 * and pointer variable name.
 *
 * It is assumed that the implementation class is named {$name}Imp
 *
 * $name        : name of the interface class
 * $parent      : name of the parent class
 * $pointer     : name of the pointer to the implementation class
 */
function interface_class( $name, $parent, $pointer ) {
    global $__grokit_if_name;
    global $__grokit_if_parent;
    global $__grokit_if_pointer;

    $__grokit_if_name = $name;
    $__grokit_if_parent = $parent;
    $__grokit_if_pointer = $pointer;
?>

// Include the base class definition
#include "<?=$parent?>.h"

// Include the implementation definition
#include "<?=$name?>Imp.h"

/** Class to provide an interface to the <?=$name?>Imp class.
 *
 *  See <?=$name?>Imp.h for a dcription of the functions and behavior
 *  of the class.
 */
class <?=$name?> : public <?=$parent?> {
public:
<?php
}

/*
 *  Ends the class definition.
 */
function interface_class_end() {
    global $__grokit_if_name;
    global $__grokit_if_parent;
    global $__grokit_if_pointer;
?>
    // the virtual destructor
    virtual ~<?=$__grokit_if_name?>() {}
};
<?php

    unset($__grokit_if_name);
    unset($__grokit_if_parent);
    unset($__grokit_if_pointer);
}

/*
 *  Creates a default constructor for the interface class.
 */
function interface_default_constructor() {
    global $__grokit_if_name;
    global $__grokit_if_parent;
    global $__grokit_if_pointer;
?>
    // Default constructor
    <?=$__grokit_if_name?>( void ) :
        <?=$__grokit_if_parent?>()
    {
        <?=$__grokit_if_pointer?> = NULL;
    }
<?php
}

/*
 *  Defines a new constructor with the given arguments.
 *
 *  $args   : the arguments to the constructor (associative array)
 */
function interface_constructor( array $args ) {
    global $__grokit_if_name;
    global $__grokit_if_parent;
    global $__grokit_if_pointer;
?>
    // Constructor (creates the implementation object)
    <?=$__grokit_if_name?>( <?=typed_args($args)?> ) {
        <?=$__grokit_if_pointer?> = new <?=$__grokit_if_name?>Imp( <?=args($args)?> );
    }
<?php
}

/*
 *  Defines a new function in the interface with the given arguments, name,
 *  and return type.
 *
 *  $name   : the name of the function.
 *  $type   : the return type of the function.
 *  $args   : the parameters to the function [ name => type, ...]
 */
function interface_function( $name, $type, $args ) {
    global $__grokit_if_name;
    global $__grokit_if_parent;
    global $__grokit_if_pointer;
?>
    <?=$type?> <?=$name?>( <?=typed_args($args)?> ) {
        <?=$__grokit_if_name?>Imp& obj = dynamic_cast< <?=$__grokit_if_name?>Imp& >( *<?=$__grokit_if_pointer?> );
        return obj.<?=$name?>( <?=args($args)?> );
    }
<?php
}

?>

<?=generatedFileHeader()?>
