<?
// Copyright 2013 Tera Insights, LLC. All Rights Reserved.
//
// Author: Christopher Dudley

/*
 *  Template Arguments:
 *
 *      [R] 'order':    Mapping of input name => ordering
 *                      Valid orderings are:
 *                          ascending: 'ASC', 'ASCENDING', '+', '>'
 *                          descending: 'DESC', 'DESCENDING', '-', '<'
 *      [O] 'limit':    Integer specifying the number of output tuples
 *                      Default is approx 4 billion
 *      [O] 'rank':     Attribute to store the position of the tuple in the
 *                      sorted order.
 */
function OrderBy( array $t_args, array $inputs, array $outputs ) {
    if( \count($inputs) == 0 ) {
        grokit_assert(array_key_exists('input', $t_args),
            'No inputs given for OrderBy');

        $inputs = $t_args['input'];

        foreach($t_args['input'] as $name => &$type) {
            if( is_identifier($type) ) {
                $type = lookupType(strval($type));
            }

            grokit_assert( is_datatype($type), 'Invalid type given for input ' . $name );
        }
    }

    grokit_assert( array_key_exists('order', $t_args), 'No ordering attributes given for OrderBy');
    $ordering = $t_args['order'];

    $ascOpts = [ 'ASC', 'ASCENDING', '+', '>' ];
    $descOpts = [ 'DESC', 'DESCENDING', 'DES', 'DSC', '-', '<' ];

    $ascending = [];
    foreach( $ordering as $name => $order ) {
        grokit_assert( array_key_exists($name, $inputs), 'Ordering attribute ' . $name . ' not present in input');

        if( in_array_icase($order, $ascOpts) ) {
            $ascending[$name] = true;
        }
        else if( in_array_icase($order, $descOpts) ) {
            $ascending[$name] = false;
        }
        else {
            grokit_error("Unknown ordering " . $order . " given for attribute " . $name );
        }
    }

    $rankAtt = get_default($t_args, 'rank', null);
    grokit_assert(is_null($rankAtt) || is_attribute($rankAtt), 'Rank argument should be null or an attribute');
    grokit_assert(is_null($rankAtt) || array_key_exists($rankAtt->name(), $outputs), 'Rank attribute does not exist in outputs');

    if( !is_null($rankAtt) && is_null($outputs[$rankAtt->name()]) ) {
        $outputs[$rankAtt->name()] = lookupType('base::BIGINT');
    }

    $outputPassthroughAtts = [];
    foreach( $outputs as $name => $type ) {
        if( is_null($rankAtt) || $rankAtt->name() != $name ) {
            $outputPassthroughAtts[$name] = $type;
        }
    }

    $outToIn = [];
    $nInputs = \count($inputs);
    reset($inputs);
    reset($outputPassthroughAtts);
    for( $i = 0; $i < $nInputs; $i++ ) {
        $outName = key($outputPassthroughAtts);
        $inName = key($inputs);

        $outToIn[$outName] = $inName;

        // Unify types
        $outputs[$outName] = $inputs[$inName];
        $outputPassthroughAtts[$outName] = $inputs[$inName];

        next($inputs);
        next($outputPassthroughAtts);
    }

    $orderAtts = [];
    $extraAtts = [];
    foreach( $inputs as $name => $type ) {
        if( array_key_exists($name, $ordering) ) {
            $orderAtts[$name] = $type;
        }
        else {
            $extraAtts[$name] = $type;
        }
    }

    // Give 2^32 as the default, which should be effectively infinite
    $limitDefault = pow(2, 32);
    $limit = get_default( $t_args, 'limit', $limitDefault );
    $limit = $limit == 0 ? $limitDefault : $limit;
    grokit_assert( $limit > 0, 'The OrderBy limit must be a positive integer');

    $className = generate_name('OrderBy');

    $debug = get_default( $t_args, 'debug', 0 );
?>

class <?=$className?> {
    struct Tuple {
<? foreach( $inputs as $name => $type ) { ?>
        <?=$type?> <?=$name?>;
<?  } ?>

        Tuple( void ) = default;

        Tuple( const Tuple & other ) = default;

        Tuple( <?=array_template('const {val} & _{key}', ', ', $inputs)?>):
            <?=array_template('{key}(_{key})', ', ', $inputs)?>

        { }

        Tuple & operator = (const Tuple & other ) = default;

        bool operator > ( const Tuple & other ) const {
<?  foreach($orderAtts as $name => $type )  {
        $op1 = $ascending[$name] ? '<' : '>';
        $op2 = !$ascending[$name] ? '<' : '>';
?>
            if( <?=$name?> <?=$op1?> other.<?=$name?> )
                return true;
            else if( <?=$name?> <?=$op2?> other.<?=$name?> )
                return false;
<?  } ?>

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

<?  if( $debug > 0 ) { ?>
        std::string toString(void) const {
            std::ostringstream ss;

            ss << "( "; // >
<?      $first = true;
        foreach( $inputs as $name => $type ) {
            if( $first ) $first = false;
            else echo '            ss << ", ";' . PHP_EOL;
?>
            ss << <?=$name?>; // >
<?      } // foreach input ?>
            ss << " )"; // >

            return ss.str();
        }
<?  } // debug > 0 ?>

    }; // struct Tuple

    typedef std::vector<Tuple> TupleVector;
public:

    class Iterator {
    public:
        typedef TupleVector::const_iterator iter_type;

    private:
        iter_type begin;
        iter_type curr;
        iter_type end;

    public:
        Iterator(void) = default;
        Iterator( const iter_type & _begin, const iter_type & _end ) : begin(_begin), curr(_begin), end(_end)
        { }

        bool GetNextResult(<?=typed_ref_args($outputs)?>) {
            if( curr != end ) {
<?  foreach($outputPassthroughAtts as $name => $type ) { ?>
                <?=$name?> = curr-><?=$outToIn[$name]?>;
<?  } ?>
<?  if( ! is_null($rankAtt) ) { ?>
                <?=$rankAtt?> = (curr - begin) + 1;
<?  } // if we need to output the rank ?>
                curr++;
                return true;
            }
            else {
                return false;
            }
        }

    };

private:

    uintmax_t __count;  // number of tuples covered

    // K, as in Top-K
    static constexpr size_t K = <?=$limit?>;

    TupleVector tuples;

    // Iterator for multi output type
    Iterator multiIterator;

    typedef std::greater<Tuple> TupleCompare;

    // Function to force sorting so that GetNext gets the tuples in order.
    void Sort(void) {
        TupleCompare comp;
        // If tuples doesn't contain at least K elements, it was never made into
        // a heap in the first place, so sort it normally.
        if( tuples.size() >= K ) {
            std::sort_heap(tuples.begin(), tuples.end(), comp);
        } else {
            std::sort(tuples.begin(), tuples.end(), comp);
        }
    }

    // Internal function to add a tuple to the heap
    void AddTupleInternal(Tuple & t ) {
<?  if( $debug >= 1 ) { ?>
        {
            std::ostringstream ss;
            ss << "T ACK: " << t.toString() << std::endl; // >
            std::cerr << ss.str(); // >
        }
<?  } ?>
        TupleCompare comp;
        if( tuples.size() >= K ) {
<?  if( $debug >= 1 ) { ?>
            {
                std::ostringstream ss;
                ss << "T REP: " << tuples.front().toString() << std::endl; // >
                std::cerr << ss.str(); // >
            }
<?  } ?>
            std::pop_heap(tuples.begin(), tuples.end(), comp);
            tuples.pop_back();
            tuples.push_back(t);
            std::push_heap(tuples.begin(), tuples.end(), comp);
        } else {
            tuples.push_back(t);
            if( tuples.size() == K ) {
                std::make_heap(tuples.begin(), tuples.end(), comp);
            }
        }
    }

public:

    <?=$className?>() : __count(0), tuples(), multiIterator()
    { }

    ~<?=$className?>() { }

    void AddItem(<?=const_typed_ref_args($inputs)?>) {
        __count++;
        Tuple t(<?=args($inputs)?>);
<?  if( $debug >= 2 ) { ?>
        {
            std::ostringstream ss;
            ss << "T NEW: " << t.toString() << std::endl; // >
            std::cerr << ss.str(); // >
        }
<?  } ?>
        if( tuples.size() == K && !(t > tuples.front()) )
            return;

        AddTupleInternal(t);
    }

    void AddState( <?=$className?> & other ) {
        __count += other.__count;
        for( Tuple & el : other.tuples ) {
            if( tuples.size() < K /*>*/ || el > tuples.front() ) {
                AddTupleInternal(el);
            }
        }
    }

    void Finalize() {
        Sort();
        Iterator::iter_type begin = tuples.cbegin();
        Iterator::iter_type end = tuples.cend();
        multiIterator = Iterator(begin, end);

<?  if( $debug >= 1 ) { ?>
        std::ostringstream ss;
        ss << "[ "; //>
        bool first = true;
        for( auto el : tuples ) {
            if( first )
                first = false;
            else
                ss << ", "; //>>

            ss << el.toString(); //>>
        }
        ss << " ]" << std::endl; // >
        std::cerr << ss.str(); //>>
<?  } ?>
    }

    bool GetNextResult( <?=typed_ref_args($outputs)?> ) {
        return multiIterator.GetNextResult(<?=args($outputs)?>);
    }
};

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
} // end function OrderBy
?>
