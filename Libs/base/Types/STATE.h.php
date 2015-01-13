<?
//
//  Copyright 2012 Alin Dobra and Christopher Jermaine, Apache 2.0
//  Copyright 2013, Tera Insights. All rights reserved

function STATE( array $t_args ){
    $type = get_first_key( $t_args, [ 'type', '0' ] );
    grokit_assert(is_gla($type),
        'Template argument to STATE must be a valid GLA.');

    $type = $type->lookup();

    $gContent = '';

    $functions = [];
    $methods = [];

    $className = generate_name('STATE_');
?>

/** Type definition for generic GLA states. This type is only used to
    trannsport states withing the same memory space between
    operators. The object the state points to MUST be treated like a
    const.

    Note: this type cannot be read from the disk or written to the
    output. A different mechanism will be used for that.

    The type in the object must be a hash of the name of the class
    used to encode the object. Any function that assumes a certain
    type must explicitly verify the correctness of the type.

    The object can be manipulated like a basic datatype. STATE objects
    do not know how to deallocate the memory they use. Other
    mechanisms have to be used to ensure correct deallocation
    (acknowledgements of data packets that contain this as members).

**/

class <?=$className?> {
public:
    typedef <?=$type?>* pointer_type;
    typedef uint64_t hash_type;

private:
    pointer_type object;
    hash_type type;

public:

    <?=$className?>():
        object(nullptr),
        type(0)
    {}
    <?=$className?>(pointer_type _object):
        object(_object),
        type(<?=$type->cHash()?>)
    {}

    pointer_type GetObject() const {
        FATALIF(type != <?=$type->cHash()?>, "STATE contains incorrect type!");
        return object;
    }

<?  $methods[] = ['IsNull', [], 'BASE::BOOL', true ]; ?>
    bool IsNull() const {
        return object == nullptr;
    }

    /** no destructor. object should not be deallocated here */
};

<?  ob_start(); ?>

<?  $functions[] = ['IsNull', ['@type'], 'BASE::BOOL', true, true]; ?>
inline
bool IsNull( const @type & d ) {
    return d.IsNull();
}

<? $gContent .= ob_get_clean(); ?>

<?

return array(
    'kind' => 'TYPE',
    'name' => $className,
    "complex" => false,
    'extras' => [ 'type' => $type ],
    'properties' => [ '__state__' ],
    'global_content' => $gContent,
    'methods' => $methods,
    'functions' => $functions
    );

} // end of function



?>
