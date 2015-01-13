<?
// Copyright 2013 Tera Insights, LLC. All rights reserved.
//
// Author: Christopher Dudley

/*
 *  Template Arguments:
 *
 *      [R] 'extremes': Mapping from input name => extreme type
 *                      Valid extreme type are:
 *                          min:    'MIN', 'MINIMUM', '-', '<'
 *                          max:    'MAX', 'MAXIMUM', '+', '>'
 */
function ExtremeTuples( array $t_args, array $inputs, array $outputs ) {
    $extremes = get_first_key($t_args, [ 'extremes' ]);
    $nExt = \count($extremes);

    grokit_assert($nExt > 0, 'No extremes specified for ExtremeTuples GLA.');

    if( \count($inputs) == 0 ) {
        grokit_assert(array_key_exists('inputs', $t_args),
            'No arguments specified for ExtremeTuples GLA.');

        $count = 0;
        foreach( $t_args['inputs'] as $type ) {
            if( is_identifier($type) ) {
                $type = lookupType(strval($type));
            }

            grokit_assert(is_datatype($type), 'Only datatypes can be specified as inputs to '.
                'the ExtremeTuples GLA');

            $name = 'et_val' . $count;
            $inputs[$name] = $type;
        }
    }

    $outputMap = [];

    reset($outputs);
    foreach( $inputs as $name => $type ) {
        $oKey = key($outputs);
        $outputs[$oKey] = $type;

        $outputMap[$oKey] = $name;

        next($outputs);
    }

    grokit_assert( $nExt <= \count($inputs),
        'There can not be more extreme values than there are inputs!');

    $mainAtts = [];
    $extraAtts = [];

    $minOpts = [ 'MIN', 'MINIMUM', '-', '<' ];
    $maxOpts = [ 'MAX', 'MAXIMUM', '+', '>' ];

    $inArrayCase = function($needle,  $haystack ) {
        foreach( $haystack as $item ) {
            if( strcasecmp($needle, $item) == 0 )
                return true;
        }
        return false;
    };

    $minimum = [];
    foreach( $extremes as $name => $val ) {
        grokit_assert(array_key_exists($name, $inputs), "ExtremeTuples: Expression with name " . $name . " specified as extreme not found in inputs");
    }

    foreach( $inputs as $name => $type ) {
        if( array_key_exists($name, $extremes) ) {
            $mainAtts[$name] = $type;

            if( $inArrayCase($extremes[$name], $minOpts) ) {
                $minimum[$name] = true;
            }
            else if( $inArrayCase($extremes[$name], $maxOpts) ) {
                $minimum[$name] = false;
            }
            else {
                grokit_error('Unknown extreme type ' . $extremes[$name] . ' specified for ' . $name);
            }
        }
        else {
            $extraAtts[$name] = $type;
        }
    }

    $debug = get_default($t_args, 'debug', 0);

    $className = generate_name('ExtremeTuples');
?>

class <?=$className?> {

    struct Tuple {
<?  foreach($inputs as $name => $type) {  ?>
        <?=$type?> <?=$name?>;
<?  } // foreach input ?>

        // Default Constructor, Copy Constructor, and Copy Assignment are all
        // default
        Tuple(void) = default;
        Tuple(const Tuple &) = default;
        Tuple & operator = (const Tuple &) = default;

        Tuple(<?=array_template('const {val} & _{key}', ', ', $inputs)?>) :
            <?=array_template('{key}(_{key})', ', ', $inputs)?>

        { }

        // operator > means that this tuple is "better" than the other tuple.
        bool operator > ( const Tuple & other ) const {
<?  foreach($mainAtts as $name => $type )  {
        $op1 = $minimum[$name] ? '<' : '>';
        $op2 = !$minimum[$name] ? '<' : '>';
?>
            if( <?=$name?> <?=$op1?> other.<?=$name?> )
                return true;
            else if( <?=$name?> <?=$op2?> other.<?=$name?> )
                return false;
<?  } // foreach main attribute ?>

            return false;
        }

        bool operator < ( const Tuple& other ) const {
            return other > *this;
        }

        bool operator <= (const Tuple & other ) const {
            return ! (*this > other );
        }

        bool operator >= (const Tuple & other ) const {
            return !( other > *this );
        }

        bool operator == (const Tuple & other ) const {
            bool ret = true;
<?  foreach($mainAtts as $name => $type )  { ?>
            ret &= <?=$name?> == other.<?=$name?>;
<?  } // foreach main attribute ?>
            return ret;
        }
    }; // struct Tuple

    typedef std::vector<Tuple> TupleVector;
public:
    class Iterator {
    public:
        typedef TupleVector::const_iterator iter_type;

    private:
        iter_type begin;
        iter_type end;

    public:
        Iterator(void) = default;
        Iterator(const Iterator &) = default;
        Iterator( const iter_type & _begin, const iter_type & _end ) : begin(_begin), end(_end)
        { }
        Iterator( const iter_type && _begin, const iter_type && _end ) : begin(_begin), end(_end)
        { }

        bool GetNextResult(<?=typed_ref_args($outputs)?>) {
            if( begin != end ) {
<?  foreach($outputs as $name => $type ) { ?>
                <?=$name?> = begin-><?=$outputMap[$name]?>;
<?  } ?>
                begin++;
                return true;
            }
            else {
                return false;
            }
        }

    };

private:

    uintmax_t __count;  // number of tuples covered

    TupleVector tuples;

    // Iterator for multi output type
    Iterator multiIterator;

public:
    // Constructor and destructor
    <?=$className?>(void) : __count(0), tuples(), multiIterator()
    { }

    ~<?=$className?>() { }

    void AddItem( <?=const_typed_ref_args($inputs)?> ) {
        ++__count;
        Tuple t(<?=args($inputs)?>);

        if( tuples.empty() ) {
            tuples.push_back(t);
        }
        else if( t > tuples.front() ) {
            tuples.clear();
            tuples.push_back(t);
        }
        else if( t == tuples.front() ) {
            tuples.push_back(t);
        }
    }

    void AddState( <?=$className?> & other ) {
        if( tuples.size() == 0 ) {
            tuples.swap(other.tuples);
        }
        else if( other.tuples.size() == 0 ) {
            // Do nothing
        }
        else if( tuples.front() > other.tuples.front() ) {
            // fast path
        }
        else if( other.tuples.front() > tuples.front() ) {
            tuples.swap(other.tuples);
        }
        else {
            for( Tuple & t : other.tuples ) {
                tuples.push_back(t);
            }
        }
    }

    void Finalize( void ) {
        multiIterator = Iterator(tuples.cbegin(), tuples.cend());
    }

    bool GetNextResult(<?=typed_ref_args($outputs)?>) {
        return multiIterator.GetNextResult(<?=args($outputs)?>);
    }
}; // class <?=$className?>

<?
    $system_headers = [ 'vector', 'algorithm', 'cinttypes' ];
    if( $debug > 0 ) {
        $system_headers = array_merge($system_headers, [ 'iostream', 'sstream', 'string' ] );
    }

    return array(
        'kind'           => 'GLA',
        'name'           => $className,
        'input'          => $inputs,
        'output'         => $outputs,
        'result_type'    => 'multi',
        'system_headers' => $system_headers,
    );
} // function ExtremeTuples
?>
