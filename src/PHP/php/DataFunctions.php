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

require_once('grokit_base.php');

class DataType
{
    private $children = [];

    // Properties taken from constructor
    private $name = "No Name";
    private $parent = null;
    private $copyArgs = [];
    private $swapArgs = [];
    private $copyable = false;
    private $allowSubs = false;
    private $serializable = false;
    private $sep_deserial = false;

    // Generated properties
    private $constructorArgs = [];
    private $myArgs = [];
    private $allCopy = [];
    private $allSwap = [];

    function __construct( $name, array $copyArgs, array $swapArgs, $copyable, $allowSubs, $serializable, $sep_deserial, $parent = null ) {
        $this->name = $name;
        $this->copyArgs = $copyArgs;
        $this->swapArgs = $swapArgs;
        $this->copyable = $copyable;
        $this->allowSubs = $allowSubs;
        $this->parent = $parent;
        $this->serializable = $serializable;
        $this->sep_deserial = $sep_deserial;

        $this->myArgs = array_merge( $this->copyArgs, $this->swapArgs );

        if ($parent != null) {
            if( get_class($parent) != get_class() ) {
                throw new \RuntimeException("Parent of datatype $name was not a valid datatype or null!");
            }

            if( $copyable != $parent->copyable ) {
                throw new \RuntimeException("Attempting to create datatype $name with different copyability of parent $parent->name");
            }

            if( ! $parent->allowSubs ) {
                throw new \RuntimeException("Attempting to create datatype $name with non-subclassable parent $parent->name");
            }

            if( $serializable && ! $parent->serializable ) {
                throw new \RuntimeException("Attempting to create serializable datatype from non-serializable parent $parent->name");
            }

            $this->constructorArgs = $parent->constructorArgs;
            $this->allCopy = $parent->copyArgs;
            $this->allSwap = $parent->swapArgs;
        }

        $this->constructorArgs = array_merge($this->constructorArgs, $this->copyArgs, $this->swapArgs);
        $this->allCopy = array_merge($this->allCopy, $this->copyArgs);
        $this->allSwap = array_merge($this->allSwap, $this->swapArgs);
    }

    public function add_child($child) {
        $this->children[] = $child;
    }

    public function get_children() {
        return $this->children;
    }

    public function is_copyable() {
        return $this->copyable;
    }

    public function get_name() {
        return $this->name;
    }

    // Returns an array of the constructor parameters, already formatted
    // properly
    private function _get_constructor_params() {
        $res = [];
        if( $this->parent != null ) {
            $res = $this->parent->_get_constructor_params();
        }

        foreach( $this->copyArgs as $n => $t ) {
            array_push($res, "$t $n");
        }

        foreach( $this->swapArgs as $n => $t ) {
            array_push($res, "$t & $n");
        }

        return $res;
    }

    // Returns a string of the typed constructor parameters
    private function get_constructor_params() {
        return implode(CSL_SEP, $this->_get_constructor_params());
    }

    public function expand() {
        // Alias members so I don't go crazy trying to add $this-> to all of
        // them. I didn't know that you HAVE to have the $this->, even within
        // a class's method.
        $name = &$this->name;
        $parent = &$this->parent;
        $copyArgs = &$this->copyArgs;
        $swapArgs = &$this->swapArgs;
        $copyable = &$this->copyable;
        $serializable = &$this->serializable;
        $sep_deserial = $this->sep_deserial;

        $constructorArgs = &$this->constructorArgs;
        $myArgs = &$this->myArgs;
        $allCopy = &$this->allCopy;
        $allSwap = &$this->allSwap;
?>

// forward definition
class <?=$name?>;

// Defining the implementation class first.
class <?=$name?>Imp : public <?=$parent->name?>Imp {

protected:
    // members
<?php foreach( $myArgs as $n => $t ) { ?>
    <?=$t?> <?=$n?>;
<?php } ?>

public:
    // Constructor
    <?=$name?>Imp( <?=$this->get_constructor_params()?> ) :
        <?=$parent->name?>Imp( <?=args($parent->constructorArgs)?> )
        // copy constructed members
<?php foreach( $copyArgs as $n => $t ) { ?>
        , <?=$n?>( <?=$n?> )
<?php } ?>
    {
<?php foreach( $swapArgs as $n => $t ) { ?>
        this-><?=$n?>.swap(<?=$n?>);
<?php } ?>
    }

<?php if( count($constructorArgs) > 0 ) { ?>
    // Default constructor
    <?=$name?>Imp() : <?=$parent->name?>Imp() { }
<?php } ?>

<?php if( $copyable ) { ?>
protected:

    // Protected copy constructor used for Clone
    <?=$name?>Imp( <?=$name?>Imp& fromMe ) : <?=$parent->name?>Imp( fromMe ) {
        // Copy my stuff
        // Copy-constructed stuff first
<?php foreach( $copyArgs as $n => $t ) { ?>
        this-><?=$n?> = fromMe.<?=$n?>;
<?php } ?>

        // Now swapped stuff (with explicit copies)
<?php foreach( $swapArgs as $n => $t ) { ?>
        this-><?=$n?>.copy( fromMe.<?=$n?> );
<?php } ?>
    }

public:
    // Clone method. Can create a copy of the object.
    virtual DataImp* Clone() OVERRIDE_SPEC {
        // Use copy constructor to create a new object that copies me
        return new <?=$name?>Imp( *this );
    }
<?php } ?>

    // destructor
    virtual ~<?=$name?>Imp() {}

    // type
    static constexpr off_t _type_ = <?=hash_name($name)?>;
    virtual const off_t Type(void) const OVERRIDE_SPEC {
        return <?=hash_name($name)?>;
    }
    virtual const char * TypeName(void) const OVERRIDE_SPEC {
        return "<?=$name?>";
    }

<?  if( $serializable ) { ?>
    virtual void toJson( Json::Value & dest ) const OVERRIDE_SPEC {
        dest = Json::Value(Json::objectValue);

        // Serialize parent
        <?=$parent->name?>Imp::toJson(dest["|parent|"]);

        dest["|type|"] = (Json::Int64) _type_;

        // Store members
<?      foreach( $myArgs as $n => $t ) { ?>
        ToJson(<?=$n?>, dest["<?=$n?>"]);
<?      } ?>
    }

    virtual void fromJson( const Json::Value & src ) OVERRIDE_SPEC {
        // Deserialize parent
        <?=$parent->name?>Imp::fromJson(src["|parent|"]);

        off_t jType = (off_t) src["|type|"].asInt64();

        FATALIF(jType != _type_, "Attempting to deserialize <?=$name?> from JSON for different type "
            "with hash %llx", (unsigned long long) jType);

        // Deserialize members
<?      foreach( $myArgs as $n => $t ) { ?>
        FromJson(src["<?=$n?>"], <?=$n?>);
<?      } ?>
    }
<?  } // if serializable
    else  { ?>
    virtual void toJson( Json::Value & dest ) const OVERRIDE_SPEC {
        FATAL("Data type <?=$name?> is not serializable.");
    }

    virtual void fromJson ( const Json::Value & src ) OVERRIDE_SPEC {
        FATAL("Data type <?=$name?> is not serializable.");
    }
<?  } // it not serializable ?>

    friend class <?=$name?>;
}; // End of class <?=$name?>Imp

// Front end class <?=$name?>

class <?=$name?> : public <?=$parent->name?> {
public:
    // the type
    static const off_t type = <?=hash_name($name)?>;

    // Constructor
    <?=$name?>( <?=$this->get_constructor_params()?> ) {
        this->data = new <?=$name?>Imp( <?=args($constructorArgs)?> );
    }

<?php if( count($constructorArgs) > 0 ) { ?>
    // Default constructor
    <?=$name?>( void ) : <?=$parent->name?>( ) { }
<?php } ?>

<?  if( $copyable ) {    ?>
    // Copy constructor
    <?=$name?>( <?=$name?> & o ) {
        copy(o);
    }

    // Copy assignment
    <?=$name?> & operator = (<?=$name?>& o) {
        copy(o);
        return *this;
    }
<?  } ?>

    // Move constructor
    <?=$name?>( <?=$name?> && o ): <?=$parent->name?>()
    {
        swap(o);
    }

    // Access methods for all new data
<?php foreach( $myArgs as $n => $t ) { ?>
    <?=$t?>& get_<?=$n?>() {
        <?=$name?>Imp * myData = dynamic_cast< <?=$name?>Imp* >( this->data );
        FATALIF( myData == nullptr, "Trying to get member <?=$n?> of an invalid or wrong type <?=$name?>!");
        return myData-><?=$n?>;
    }
<?php } ?>

    virtual void toJson( Json::Value & dest ) const override {
        <?=$name?>Imp * myData = dynamic_cast< <?=$name?>Imp* >( this->data );
        if( myData != nullptr )
            myData->toJson(dest);
    }

    virtual void fromJson( const Json::Value & src ) override {
        if( this->data == nullptr ) {
            <?=$name?>Imp * myData = new <?=$name?>Imp;
            myData->fromJson(src);
            this->data = myData;
        }
        else {
            <?=$name?>Imp * myData = dynamic_cast< <?=$name?>Imp* >( this->data );
            FATALIF(myData == nullptr, "Trying to deserialize into invalid <?$name?> object!");
            myData->fromJson(src);
        }
    }

}; // End of class <?=$name?>

inline
void ToJson( const <?=$name?> & src, Json::Value & dest ) {
    src.toJson(dest);
}

<? if( !$sep_deserial ) { ?>
inline
void FromJson( const Json::Value & src, <?=$name?> & dest ) {
    dest.fromJson(src);
}
<?  } // if not separate deserializer  ?>

<?php
    } // End of expand()

    function generate_deserializer($children) {
        $name = $this->name;

?>
inline
void FromJson( const Json::Value & src, <?=$name?> & dest ) {
    FATALIF(!src.isMember("|type|"), "Attempting to deserialize <?=$name?> from invalid JSON");
    off_t jType = (off_t) src["|type|"].asInt64();

    switch( jType ) {
        case <?=$name?>::type:
            // Type is correct
            dest.fromJson(src);
            break;
        // Cases for subtypes
<?  foreach( $children as $child ) {
?>
        case <?=$child?>::type:
            {
                <?=$child?> tmp;
                FromJson( src, tmp );
                tmp.swap(dest);
            }
            break;
<?  } // for each child ?>
        default:
            FATAL("Cannot deserialize JSON with type %llx, it is not a known subtype of <?=$this->name?>", (unsigned long long) jType);
    } // end switch
}
<?
    } // end of function generate_deserializer
} // End of class DataType

// Array holding known types so far. Start out with Data and DataC since they
// are special.
$_def_types = array(
    "Data" => new DataType( "Data", [], [], false, true, true, false ),
    "DataC" => new DataType( "DataC", [], [], true, true, true, false )
);

// Internal function to create a new data type.
//  $name       : the name of the new data type.
//  $parent     : the name of the parent
//  $copyArgs   : array of copy constructed arguments (name => type)
//  $swapArgs   : array of swapped arguments (name => type)
//  $allowSubs  : boolean (whether or not to allow subtypes of this type)
//  $serializable   : Whether or not to generate JSON serialization for this type
//  $sep_deserial   : If true, deserializer function generated later
function _create_data_type( $name, $parent, array $copyArgs, array $swapArgs, $allowSubs, $serializable,
        $sep_deserial ) {
    global $_def_types;

    if( !isset($_def_types[$parent]) ) {
        throw new \RuntimeException("Attempting to create datatype with unknown parent type $parent.");
    }

    if( isset($_def_types[$name]) ) {
        throw new \RuntimeException("Attempting to create duplicate datatype $name");
    }

    $pInfo = &$_def_types[$parent];
    $copyable = $pInfo->is_copyable();

    $nType = new DataType($name, $copyArgs, $swapArgs, $copyable, $allowSubs, $serializable, $sep_deserial, $pInfo);
    $_def_types[$name] = &$nType;

    $_def_types[$parent]->add_child($name);

    $nType->expand();
}

// Creates a new subclassable datatype
function create_base_data_type( $name, $parent, array $copyArgs, array $swapArgs, $serializable = false,
        $sep_deserial = false ) {
    _create_data_type( $name, $parent, $copyArgs, $swapArgs, true, $serializable, $sep_deserial );
}

// Creates a new, non-subclassable datatype
function create_data_type( $name, $parent, array $copyArgs, array $swapArgs, $serializable = false,
        $sep_deserial = false ) {
    _create_data_type( $name, $parent, $copyArgs, $swapArgs, false, $serializable, $sep_deserial );
}

function generate_deserializer($name) {
    global $_def_types;

    if( !array_key_exists($name, $_def_types) ) {
        throw new \RuntimeException("Attempting to generate deserializer for unknown type.");
    }

    $children = [];
    $flatten = function(&$val, $key) use (&$flatten, &$_def_types, &$children) {
        $children[] = $val;
        array_walk($_def_types[$val]->get_children(), $flatten);
    };
    array_walk($_def_types[$name]->get_children(), $flatten);

    $_def_types[$name]->generate_deserializer($children);
}

?>

<?=generatedFileHeader()?>

#include "Config.h"
#include "Swap.h"
#include "Data.h"
#include "SerializeJson.h"
