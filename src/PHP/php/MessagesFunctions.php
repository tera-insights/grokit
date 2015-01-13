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
 * This file contains the required functions and classes to create new types
 * of messages.
 *
 * This is like a much simpler version of DataFunctions.php, as messages do
 * not need to be subclassed.
 */
require_once('grokit_base.php');

function should_reference( $type ) {
    static $_noref_types = [ 'int', 'bool', 'double', 'float', 'long', 'long long' ];

    if( in_array(strtolower($type), $_noref_types) ) {
        return false;
    }

    return true;
}

function create_message_type( $name, array $copyArgs, array $swapArgs, $serializable = false,
    $copyable = false )
{
    $allArgs = array_merge( $copyArgs, $swapArgs );

    // Create the constructor parameter string. We have to do this specially
    // because copy constructed parameters can be passed as const references
    // while swapped members must be passed as plain references.
    $constructor_array = [];
    foreach( $copyArgs as $n => $t ) {
        if( should_reference($t) ) {
            array_push($constructor_array, "$t & $n");
        }
        else {
            array_push($constructor_array, "$t $n");
        }
    }
    foreach( $swapArgs as $n => $t ) {
        array_push($constructor_array, "$t & $n");
    }

    $constructor_params = implode(CSL_SEP, $constructor_array);

    // The factory function takes an EventProcessor as its first input, so
    // put together the parameters for that.
    $factory_array = [ 'EventProcessor & __dest' ];
    $factory_array = array_merge( $factory_array, $constructor_array );
    $factory_params = implode(CSL_SEP, $factory_array);

    $allArgs = array_merge($copyArgs, $swapArgs);

    // Escape from PHP to start outputing the C++ code.
?>

class <?=$name?> : public Message {
public:
    // members
<?php foreach( $allArgs as $n => $t ) { ?>
    <?=$t?> <?=$n?>;
<?php } ?>

private:
    // constructor
    <?=$name?> ( <?=$constructor_params?> ) :
        Message()
        // Copy constructed members
<?php foreach( $copyArgs as $n => $t ) { ?>
        , <?=$n?>( <?=$n?> )
<?php } ?>
    {
        // swapped members
<?php foreach( $swapArgs as $n => $t ) {?>
        (this-><?=$n?>).swap(<?=$n?>);
<?php } ?>
    }

<?php if( count($constructor_array) > 0 ) { ?>
    // Default constructor
    <?=$name?> ( void ) : Message() { }
<?php } ?>

<?  if( $copyable ) { ?>
    <?=$name?>( <?=$name?> & other ) {
<?  foreach( $copyArgs as $n => $t ) {?>
        <?=$n?> = other.<?=$n?>;
<?  } ?>
<?  foreach( $swapArgs as $n => $t ) {?>
        <?=$n?>.copy(other.<?=$n?>);
<?php } ?>
    }
<?  } // if copyable ?>

public:
    // Destructor
    virtual ~<?=$name?>() {}

    // type
    static constexpr off_t type = <?=hash_name($name)?>;
    virtual off_t Type(void) const OVERRIDE_SPEC { return <?=hash_name($name)?>; }
    virtual const char * TypeName(void) const OVERRIDE_SPEC { return "<?=$name?>"; }

    // To/From Json
<?  if( $serializable ) { ?>
    virtual void ToJson( Json::Value & dest ) const OVERRIDE_SPEC {
        dest = Json::Value(Json::objectValue);

<?      foreach( $copyArgs as $n => $t ) { ?>
        ::ToJson(<?=$n?>, dest["<?=$n?>"]);
<?      } ?>
<?      foreach( $swapArgs as $n => $t ) { ?>
        ::ToJson(<?=$n?>, dest["<?=$n?>"]);
<?      } ?>
    }

    virtual void FromJson ( const Json::Value & src ) OVERRIDE_SPEC {
        if( ! src.isObject() ) {
            throw new std::invalid_argument("Tried to construct <?=$name?> message from non-object JSON");
        }

<?      foreach( $allArgs as $n => $t ) { ?>
        if( ! src.isMember("<?=$n?>") )
            throw new std::invalid_argument("JSON for message <?=$name?> has no member for attribute <?=$n?>");
        ::FromJson(src["<?=$n?>"], <?=$n?>);
<?      } ?>
    }

<?  } // if serializable
    else  { ?>
    virtual void ToJson( Json::Value & dest ) const OVERRIDE_SPEC {
        FATAL("Message type <?=$name?> is not serializable.");
    }

    virtual void FromJson ( const Json::Value & src ) OVERRIDE_SPEC {
        FATAL("Message type <?=$name?> is not serializable.");
    }
<?  } // it not serializable ?>

    // Constructor from JSON
    // This constructor has a bizarre signature on purpose as not to conflict
    // with messages that contain exactly 1 JSON value as their payload.
    // It is our hope that no sane individual would store 3 void pointers in a
    // message.
    <?=$name?>( const Json::Value & src, void * dummy1, void * dummy2, void * dummy3 ) {
        FromJson(src);
    }

    // friend delcarations
<?php if(isset($message_debug_class)) { ?>
    friend class <?=$message_debug_class?>;
<?php } ?>
    friend void <?=$name?>_Factory( <?=$factory_params?> );

    // Factory function to build a <?=$name?> object
    static void Factory( <?=$factory_params?> ) {
        Message * __msg = (Message *) new <?=$name?>( <?=args($allArgs)?> );
        __dest.ProcessMessage(*__msg);
    }

<?  if( $copyable ) { ?>
    // Factory function to send a copy of the message
    static void Factory( EventProcessor & dest, <?=$name?> & msg ) {
        Message * __msg = (Message *) new <?=$name?>( msg );
        dest.ProcessMessage(*__msg);
    }
<?  } // if copyable ?>
}; // End of class <?=$name?>

inline
void ToJson( const <?=$name?> & src, Json::Value & dest ) {
    src.ToJson(dest);
}

inline
void FromJson( const Json::Value & src, <?=$name?> & dest ) {
    dest.FromJson(src);
}


// Factory function to build <?=$name?> objects
inline
void <?=$name?>_Factory( <?=$factory_params?> ) {
    Message * __msg = (Message *) new <?=$name?>( <?=args($allArgs)?> );
    __dest.ProcessMessage(*__msg);
}

<?php
} // End of function create_message_type()
?>

<?=generatedFileHeader()?>

#include "Swap.h"
#include "Message.h"
#include "SerializeJson.h"
