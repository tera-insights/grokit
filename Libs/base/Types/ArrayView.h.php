<?php
// Coypright 2015 Tera Insights, LLC.

/**
 * A fixed-size typed array view on top of memory. The view is read-only
 *
 * This is used to prevent copying of data when extracting from a column.
 */
function FixedArrayView(array $t_args) {
	$constructors = [];
	$methods = [];
	$functions = [];
	$globalContent = '';

	grokit_assert(array_key_exists('type', $t_args), 'FixedArrayView: No type given.');
    grokit_assert(array_key_exists('size', $t_args), 'FixedArrayView: No size given');

	$type = $t_args['type'];
	$size = $t_args['size'];

	if (is_array($type))
		$type = call_user_func_array('lookupType', $type);
	else
		$type = $type->lookup();

	grokit_assert(is_datatype($type), 'arrayView: [type] argument must be a valid datatype.');
	grokit_assert($type->isFixedSize(), 'FixedArray: variable-sized types not supported');
	grokit_assert(is_int($size), 'FixedArrayView: [size] argument must be an integer');
    grokit_assert($size > 0, 'FixedArrayView: [size] arugment must be a positive number.');

	$className = generate_name('FixedArrayView_' . $size . '_');

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

    const_pointer __elems_;

	<?=$className?>(): __elems_(nullptr) { }

	// Constructor from externally managed memory
	<?=$className?>(const_pointer ptr): __elems_(ptr) { }

	// Default copy and move constructors/assignment
	<?=$className?>(const <?=$className?> &other) = default;
	<?=$className?> & operator=(const <?=$className?> &other) = default;
	<?=$className?>(<?=$className?> &&other) = default;
	<?=$className?> & operator=(<?=$className?> &&other) = default;

	/***** Element Access *****/

<?  $methods[] = ['at', [ 'base::BIGINT' ], $type->value(), true ]; ?>
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

    const_reference operator[]( size_type pos ) const {
        return __elems_[pos];
    }

<?  $methods[] = ['front', [], $type->value(), true ]; ?>
    const_reference front() const {
        return __elems_[0];
    }

<?  $methods[] = ['back', [], $type->value(), true ]; ?>
    const_reference back() const {
        return __elems_[SIZE-1];
    }

    const_pointer data() const noexcept {
        return __elems_;
    }

	/***** Iterators *****/

    const_iterator cbegin() const noexcept {
        return __elems_;
    }

    const_iterator begin() const noexcept {
        return cbegin();
    }

    const_iterator cend() const noexcept {
        return __elems_ + size();
    }

    const_iterator end() const noexcept {
        return cend();
    }

    const_reverse_iterator crbegin() const noexcept {
        return const_reverse_iterator(cend());
    }

    const_reverse_iterator rbegin() const noexcept {
        return crbegin();
    }

    const_reverse_iterator crend() const noexcept {
        return const_reverse_iterator(cbegin());
    }

    const_reverse_iterator rend() const noexcept {
        return crend();
    }

	/***** Capacity *****/

<?  $methods[] = ['empty', [], 'base::bool', true]; ?>
    bool empty() const noexcept {
        return SIZE == 0;
    }

<?  $methods[] = ['size', [], 'base::BIGINT', true]; ?>
    size_type size() const noexcept {
        return SIZE;
    }

    size_type max_size() const noexcept {
        return SIZE;
    }

    /***** Operations *****/

    void swap( <?=$className?> & other ) noexcept {
        std::swap( __elems_, other.__elems_ );
    }

    /***** EXTENTIONS *****/

    void from_memory( const_pointer mem ) {
        __elems_ = mem;
    }
};

<?  ob_start(); ?>

inline
bool operator == ( const @type & lhs, const @type & rhs ) {
	// Fast-track for views referring to the same memory
	if (lhs.__elems_ == rhs.__elems_)
		return true;

    for( @type::size_type i = 0; i < @type::SIZE; i++ ) {
        if( lhs[i] != rhs[i] ) return false;
    }
    return true;
}

inline
bool operator != ( const @type & lhs, const @type & rhs ) {
	// Fast-track for views referring to the same memory
	if (lhs.__elems_ == rhs.__elems_)
		return false;

    for( @type::size_type i = 0; i < @type::SIZE; i++ ) {
        if( lhs[i] != rhs[i] ) return true;
    }
    return false;
}

inline
bool operator < ( const @type & lhs, const @type & rhs ) {
    return std::lexicographical_compare(lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend());
}

inline
bool operator > ( const @type & lhs, const @type & rhs ) {
    return rhs < lhs;
}

inline
bool operator <= ( const @type & lhs, const @type & rhs ) {
    return !(lhs > rhs);
}

inline
bool operator >=( const @type & lhs, const @type & rhs ) {
    return !(lhs < rhs);
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

template<>
inline
std::size_t SizeFromBuffer<@type>(const char *buffer) {
	return @type::SIZE * sizeof(@type::value_type);
}

template<>
inline
std::size_t SerializedSize(const @type& from) {
	return @type::SIZE * sizeof(@type::value_type);
}

template<>
inline
std::size_t Serialize(char *buffer, const @type &from) {
	@type::pointer ptr = reinterpret_cast<@type::pointer>(buffer);
	std::copy(from.cbegin(), from.cend(), ptr);
	return SerializedSize(from);
}

template<>
inline
std::size_t Deserialize(const char *buffer, @type &dest) {
	@type::const_pointer ptr = reinterpret_cast<@type::const_pointer>(buffer);
	dest.from_memory(ptr);
	return SizeFromBuffer<@type>(buffer);
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

// std::swap specializations

inline
void swap( @type& lhs, @type& rhs ) {
    lhs.swap(rhs);
}

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

	$sys_headers = [
		'iterator', 'algorithm', 'stdexcept', 'utility', 'cstdint',
		'cstddef', 'iostream', 'sstream', 'cstring'
		];
	$user_headers = [ 'Config.h' ];

	$extras = [
        'size' => $size,
		'type' => $type
	];

	$sizeBytes = $size * $type->get('size.bytes');
    $extras['size.bytes'] = $sizeBytes;

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
        'complex'           => "ColumnIterator<@type, 0, {$sizeBytes}>",
        'properties'        => [ 'container', 'sequence', 'array-view' ],
        'extras'            => $extras,
        'describe_json'     => DescribeJson('array', $innerDesc),
    ];
}
?>