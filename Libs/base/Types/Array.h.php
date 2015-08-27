<?
// Copyright 2014 Tera Insights, LLC. All Rights Reserved.

/**
 *  A fixed array containing a given type.
 *
 *  This is very similar to the STL array datatype, except that the size is not
 *  allowed to be 0.
 */
function FixedArray(array $t_args) {
    $constructors = [];
    $methods = [];
    $functions = [];
    $globalContent = '';

    grokit_assert(array_key_exists('type', $t_args), 'FixedArray: No type given for elements');
    grokit_assert(array_key_exists('size', $t_args), 'FixedArray: No size given');

    $type = $t_args['type'];
    $size = $t_args['size'];

    if( is_array($type) ) {
        // Perform type lookup
        $type = call_user_func_array('lookupType', $type);
    } else {
        $type = $type->lookup();
    }

    grokit_assert(is_datatype($type), 'FixedArray: [type] argument must be a valid datatype');
    grokit_assert($type->isFixedSize(), 'FixedArray: variable-sized types not supported');
    grokit_assert(is_int($size), 'FixedArray: [size] argument must be an integer');
    grokit_assert($size > 0, 'FixedArray: [size] arugment must be a positive number.');

    $className = generate_name('FixedArray_' . $size . '_');
?>
struct <?=$className?> {
    using value_type             = <?=$type?>;
    using size_type              = std::size_t;
    using difference_type        = std::ptrdiff_t;

    using reference              = value_type &;
    using const_reference        = const value_type &;
    using pointer                = value_type *;
    using const_pointer          = const value_type *;
    using iterator               = value_type *;
    using const_iterator         = const value_type *;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    static constexpr const size_type SIZE = <?=$size?>;

    value_type __elems_[SIZE > 0 ? SIZE : 1];

    // No explicit contruct/copy/destroy for aggregate type
<?  $constructors[] = [[], true]; ?>

    /***** Element Access *****/

<?  $methods[] = ['at', [ 'base::BIGINT' ], $type->value(), true ]; ?>
    reference at( size_type pos ) {
        if( size() <= pos ) {
            std::ostringstream ss;
            ss  << "Element access out of range:"
                << " size=" << size()
                << " index=" << pos;

            throw std::out_of_range(ss.str());
        }
        return __elems_[pos];
    }

    const_reference at( size_type pos ) const {
        if( size() <= pos ) {
            std::ostringstream ss;
            ss  << "Element access out of range:"
                << " size=" << size()
                << " index=" << pos;

             throw std::out_of_range(ss.str());
        }
        return __elems_[pos];
    }

    reference operator[]( size_type pos ) {
        return __elems_[pos];
    }

    constexpr const_reference operator[]( size_type pos ) const {
        return __elems_[pos];
    }

<?  $methods[] = ['front', [], $type->value(), true ]; ?>
    reference front() {
        return __elems_[0];
    }

    constexpr const_reference front() const {
        return __elems_[0];
    }

<?  $methods[] = ['back', [], $type->value(), true ]; ?>
    reference back() {
        return __elems_[SIZE-1];
    }

    constexpr const_reference back() const {
        return __elems_[SIZE-1];
    }

    pointer data() noexcept {
        return __elems_;
    }

    const_pointer data() const noexcept {
        return __elems_;
    }

    /***** Iterators *****/

    iterator begin() noexcept {
        return __elems_;
    }

    const_iterator cbegin() const noexcept {
        return __elems_;
    }

    const_iterator begin() const noexcept {
        return cbegin();
    }

    iterator end() noexcept {
        return __elems_ + size();
    }

    const_iterator cend() const noexcept {
        return __elems_ + size();
    }

    const_iterator end() const noexcept {
        return cend();
    }

    reverse_iterator rbegin() noexcept {
        return reverse_iterator(end());
    }

    const_reverse_iterator crbegin() const noexcept {
        return const_reverse_iterator(cend());
    }

    const_reverse_iterator rbegin() const noexcept {
        return crbegin();
    }

    reverse_iterator rend() noexcept {
        return reverse_iterator(begin());
    }

    const_reverse_iterator crend() const noexcept {
        return const_reverse_iterator(cbegin());
    }

    const_reverse_iterator rend() const noexcept {
        return crend();
    }

    /***** Capacity *****/

<?  $methods[] = ['empty', [], 'base::bool', true]; ?>
    constexpr bool empty() const noexcept {
        return SIZE == 0;
    }

<?  $methods[] = ['size', [], 'base::BIGINT', true]; ?>
    constexpr size_type size() const noexcept {
        return SIZE;
    }

    constexpr size_type max_size() const noexcept {
        return SIZE;
    }

    /***** Operations *****/

    void fill( const value_type & value ) {
        std::fill_n(begin(), SIZE, value);
    }

    void swap( <?=$className?> & other ) noexcept(noexcept(std::swap(std::declval<value_type&>(), std::declval<value_type&>()))) {
        std::swap( __elems_, other.__elems_ );
    }

    /***** EXTENTIONS *****/

    void from_memory( const_pointer mem ) {
        std::copy(mem, mem+SIZE, __elems_);
    }
};

<?  ob_start(); ?>

inline
bool operator == ( const @type & lhs, const @type & rhs ) {
    for( @type::size_type i = 0; i < @type::SIZE; i++ ) { //>
        if( lhs[i] != rhs[i] ) return false;
    }
    return true;
}

inline
bool operator != ( const @type & lhs, const @type & rhs ) {
    for( @type::size_type i = 0; i < @type::SIZE; i++ ) { //>
        if( lhs[i] != rhs[i] ) return true;
    }
    return false;
}

inline
bool operator < ( const @type & lhs, const @type & rhs ) { //>
    return std::lexicographical_compare(lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend());
}

inline
bool operator > ( const @type & lhs, const @type & rhs ) {
    return rhs < lhs; //>
}

inline
bool operator <= ( const @type & lhs, const @type & rhs ) { //>
    return !(lhs > rhs);
}

inline
bool operator >=( const @type & lhs, const @type & rhs ) {
    return !(lhs < rhs); //>
}

// ostream operator for easier debugging.
template<class CharT, class Traits = std::char_traits<CharT>>
std::basic_ostream<CharT, Traits>& operator << ( std::basic_ostream<CharT, Traits> & os, const @type s ) {
    std::ostringstream ss;
    bool first = true;
    ss << "[";

    for( const auto & elem : s ) {
        if( first ) {
            first = false;
        } else {
            ss << ", ";
        }

        ss << elem;
    }

    ss << "]";

    os << ss.str();
    return os;
}

inline
void ToJson( const @type & src, Json::Value & dest ) {
    dest = Json::Value(Json::arrayValue);
    for( @type::const_reference elem : src ) {
        Json::Value tmp;
        ToJson( elem, tmp );
        dest.append(tmp);
    }
}

inline
void FromJson( const Json::Value & src, @type & dest ) {
    FATALIF(!src.isArray(), "Attempted to read array from non-array JSON");
    FATALIF(!(src.size() == @type::SIZE), "Invalid number of elements in JSON for Array");

    for( Json::ArrayIndex i = 0; i < @type::SIZE; i++ ) { //>
        FromJson( src[i], dest[i] );
    }
}

inline
int ToString( const @type & x, char * buffer ) {
<?  if( $size > 0 ) { ?>
    char * start = buffer;
    char * current = start;

    for( const auto & val : x ) {
        current += ToString( val, current );

        // Replace null with space
        *(current-1) = ' ';
    }

    // Replace final comma with null
    *(current-1) = '\0';

    return current - start;
<?  } else { // if size > 0 ?>
    buffer[0] = '\0';

    return 1;
<?  } // if size == 0 ?>
}

inline
void FromString( @type & x, const char * buffer ) {
    char * current = NULL;
    char * saveptr = NULL;
    const char * delim = " ";
    char * copy = strdup(buffer);

    current = strtok_r(copy, delim, &saveptr);

    for( auto & val : x ) {
        FATALIF(current == NULL, "Not enough elements in string representation of array");
        ToString(val, current);
        current = strtok_r(NULL, delim, &saveptr);
    }

    free((void *) copy);
}

<?  $functions[] = ['Hash', ['@type'], 'BASE::BIGINT', true, true ]; ?>
inline
uint64_t Hash( const @type & val ) {
    uint64_t hashVal = H_b;
    for( @type::const_reference elem : val ) {
        hashVal = CongruentHash(Hash(elem), hashVal);
    }
    return hashVal;
}

namespace std {

#ifdef _HAS_STD_HASH

// C++11 STL-compliant hash struct specialization
template <>
class hash<@type> {
public:
    size_t operator () (const @type& key) const {
        return Hash(key);
    }
};
#endif // _HAS_STD_HASH

// std::get specializations

template< size_t I >
constexpr @type::reference get( @type& a ) {
    static_assert(I < @type::SIZE,
        "Index out of bounds for std::get(@type)");

    return a.__elems_[I];
}

template< size_t I >
constexpr @type::value_type&& get( @type&& a ) {
    static_assert(I < @type::SIZE,
        "Index out of bounds for std::get(@type)");

    return std::move(a.__elems_[I]);
}

template< size_t I >
constexpr @type::const_reference get( const @type& a ) {
    static_assert(I < @type::SIZE,
        "Index out of bounds for std::get(@type)");

    return a.__elems_[I];
}

// std::swap specializations

inline
void swap( @type& lhs, @type& rhs ) {
    lhs.swap(rhs);
}

// std::tuple_size

template<>
class tuple_size< @type > :
    public integral_constant<size_t, @type::SIZE>
{ };

// std::tuple_element

template<size_t I>
struct tuple_element< I, @type > {
    using type = @type::value_type;
};

}


<?  $globalContent .= ob_get_clean(); ?>

<?
    $innerDesc = function($var, $myType) use($type) {
        $describer = $type->describer('json');
?>
        <?=$var?>["size"] = Json::Int64(<?=$myType?>::SIZE);
<?
        $innerVar = "{$var}[\"inner_type\"]";
        $describer($innerVar, $type);
    };

    $sys_headers = [ 'iterator', 'algorithm', 'stdexcept', 'utility', 'cinttypes', 'cstddef',  'iostream', 'sstream', 'cstring', 'cstdlib' ];
    $user_headers = [ 'Config.h' ];
    $extras = [
        'size' => $size,
        'type' => $type
    ];

    if( $type->has('size.bytes') ) {
        $extras['size.bytes'] = $size * $type->get('size.bytes');
    }

    return [
        'kind'              => 'TYPE',
        'name'              => $className,
        'system_headers'    => $sys_headers,
        'user_headers'      => $user_headers,
        'constructors'      => $constructors,
        'methods'           => $methods,
        'functions'         => $functions,
        'binary_operators'  => [ '==', '!=', '<', '>', '<=', '>=' ],
        'global_content'    => $globalContent,
        'complex'           => false,
        'properties'        => [ 'container', 'sequence', 'array' ],
        'extras'            => $extras,
        'describe_json'     => DescribeJson('array', $innerDesc),
    ];
}

// Shortcut for array
declareType('Array', 'base::FixedArray', []);
?>
