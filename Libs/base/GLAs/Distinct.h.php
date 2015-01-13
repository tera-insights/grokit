<?
// Copyright 2014 Tera Insights, LLC.  All Rights Reserved.

/**
 *  A GLA that determines the distinct values of a dataset.
 */
function Distinct(array $t_args, array $input, array $output) {
    grokit_assert(\count($input) == \count($output),
        'Distinct must have the same outputs as inputs.');

    $outputsToInputs = [];
    $i = 0;
    foreach( $input as $name => $type ) {
        $outputsToInputs[array_keys($output)[$i]] = $name;
        array_set_index($output, $i++, $type);
    }

    $useMCT = get_default($t_args, 'use.mct', true);
    $initSize = get_default($t_args, 'init.size', 65536);
    $keepHashes = get_default($t_args, 'mct.keep.hashes', false);
    $fragmentSize = get_default($t_args, 'fragment.size', 100000);
    $nullCheck = get_default($t_args, 'null.check', false);

    grokit_assert(is_bool($useMCT), 'Distinct use.mct argument must be boolean');
    grokit_assert(is_integer($initSize), 'Distinct init.size argument must be an integer');
    grokit_assert($initSize > 0, 'Distinct init.size argument must be positive');
    grokit_assert(is_bool($keepHashes), 'Distinct mct.keep.hashes argument must be boolean');
    grokit_assert(is_integer($fragmentSize), 'Distinct fragment.size argument must be integral');
    grokit_assert($fragmentSize > 0, 'Distinct fragment.size argumenst must be positive');

    $nullable = [];
    if( is_bool($nullCheck) ) {
        foreach( $input as $name => $type ) {
            $nullable[$name] = $nullCheck;
        }
    } else if( is_array($nullCheck) ) {
        foreach( $input as $name => $type ) {
            $nullable[$name] = false;
        }

        foreach( $nullCheck as $index => $n ) {
            grokit_assert(is_string($n), 'Distinct null.check has invalid value at position ' . $index);
            grokit_assert(array_key_exists($n, $nullable), 'Distinct null.check has unknown input ' . $n . ' at position ' . $index);

            $nullable[$n] = true;
        }
    } else {
        grokit_error('Distinct null.check must be boolean or list of inputs to check for nulls');
    }

    $keepHashesText = $keepHashes ? 'true' : 'false';

    $system_headers = [ 'cinttypes', 'functional', 'vector' ];

    if( $useMCT ) {
        $system_headers[] = 'mct/hash-set.hpp';
        $definedSet = "mct::closed_hash_set<Key, HashKey, std::equal_to<Key>, std::allocator<Key>, {$keepHashesText}>";
    } else {
        $system_headers[] = 'unordered_map';
        $definedSet = "std::unordered_set<Key, HashKey, std::equal_to<Key>, std::allocator<Key>>";
    }

    $className = generate_name('Distinct');
?>
class <?=$className?> {

    public:
    // Value being placed into the set.
    struct Key {
<?  foreach($input as $name => $type) { ?>
        <?=$type?> <?=$name?>;
<?  } // for each input ?>

        // Construct the value by copying all of the attributes.
        Key(<?=const_typed_ref_args($input)?>) :
<?
    $first = true;
    foreach($input as $name => $type) {
?>
            <?=$first ? ' ' : ','?> <?=$name?>(<?=$name?>)
<?
        $first = false;
    } // for each input
?>
        { }

        bool operator==(const Key & o ) const {
            return true <?=array_template("&& ({key} == o.{key})", ' ', $input)?>;
        }

        size_t hash_value() const {
            uint64_t hash = H_b;
<?  foreach($input as $name => $type) { ?>
            hash = CongruentHash(Hash(<?=$name?>), hash);
<?  } // for each input ?>
            return size_t(hash);
        }
    };

    // Hashing functor for our value
    struct HashKey {
        size_t operator()(const Key& o) const {
            return o.hash_value();
        }
    };

    using Set = <?=$definedSet?>;

    // Iterator object used in multi and fragment result types
    class Iterator {
        public:
        using iterator_t = Set::const_iterator;

        private:

        iterator_t start;
        iterator_t end;

        public:

        Iterator() : start(), end() { }

        Iterator( const iterator_t & _start, const iterator_t & _end ) :
            start(_start), end(_end)
        { }

        Iterator( const Iterator & o ) : start(o.start), end(o.end)
        { }

        bool GetNextResult(<?=typed_ref_args($output)?>) {
            if( start != end ) {
<?  foreach($output as $name => $type) { ?>
                <?=$name?> = start-><?=$outputsToInputs[$name]?>;
<?  } // for each output ?>
                start++;
                return true;
            } else {
                return false;
            }
        }
    };

    private:

    // Constants
    static constexpr size_t INIT_SIZE = <?=$initSize?>;
    static constexpr size_t FRAG_SIZE = <?=$fragmentSize?>;

    // Member variables

    uint64_t count;         // Total # tuples seen

    Set distinct;           // Set of distinct values

    using IteratorList = std::vector<Iterator>;

    Iterator multiIterator;     // Internal iterator for multi result type
    IteratorList fragments;     // Iterator for fragments

    public:

    <?=$className?>() :
        count(0),
        distinct(INIT_SIZE),
        multiIterator(),
        fragments()
    { }

    ~<?=$className?>() { }

    void Reset(void) {
        count = 0;
        distinct.clear();
    }

    void AddItem(<?=const_typed_ref_args($input)?>) {
        count++;
<?  foreach( $nullable as $name => $check ) { ?>
<?      if( $check ) { ?>
        if( IsNull( <?=$name?> ) ) return;
<?      } // if checking for nulls ?>
<?  } // foreach input ?>

        Key key(<?=args($input)?>);

        distinct.insert(key);
/*
        auto it = distinct.find(key);
        if( it == distinct.end() ) {
            distinct.insert(key);
        }
*/
    }

    void AddState( <?=$className?> & other ) {
        for( auto & elem : other.distinct ) {
            distinct.insert(elem);
            /*
            auto it = distinct.find(elem);
            if( it == distinct.end() ) {
                distinct.insert(elem);
            }
            */
        }
        count += other.count;
    }

    // Multi interface
    void Finalize(void) {
        multiIterator = Iterator(distinct.cbegin(), distinct.cend());
    }

    bool GetNextResult(<?=typed_ref_args($output)?>) {
        return multiIterator.GetNextResult(<?=args($output)?>);
    }

    // Fragment interface
    int GetNumFragments(void) {
        fragments.clear();
        int nFrag = 0;

        Iterator::iterator_t prev = distinct.cbegin();
        Iterator::iterator_t end = distinct.cend();
        Iterator::iterator_t next = prev;

        while( next != end ) {
            for( size_t i = 0; next != end && FRAG_SIZE > i; i++ ) {
                next++;
            }
            Iterator nIter(prev, next);
            fragments.push_back(nIter);

            prev = next;
            nFrag++;
        }

        return nFrag;
    }

    Iterator * Finalize(int fragment) {
        return new Iterator(fragments[fragment]);
    }

    bool GetNextResult(Iterator * it, <?=typed_ref_args($output)?>) {
        return it->GetNextResult(<?=args($output)?>);
    }

    // General methods
    uint64_t get_count() const {
        return count;
    }

    uint64_t get_countDistinct() const {
        return distinct.size();
    }

    const Set & get_distinct() const {
        return distinct;
    }
};

typedef <?=$className?>::Iterator <?=$className?>_Iterator;
<?
    return [
        'kind'          => 'GLA',
        'name'          => $className,
        'input'         => $input,
        'output'        => $output,
        'result_type'   => [ 'multi', 'fragment' ],
        'user_headers'  => [ 'HashFunctions.h' ],
        'system_headers' => $system_headers,
        'properties'    => [ 'resettable' ],
    ];
}
?>
