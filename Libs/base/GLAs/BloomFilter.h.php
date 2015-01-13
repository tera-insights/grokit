<?
// Copyright 2014 Tera insights, LLC. All Rights Reserved.

/**
 *  A GLA that estimates the cardinality of a dataset using a bloom filter of
 *  a configurable size.
 *
 *  Note: This filter has very high performance, so long as all of the states
 *  fit into cache, preferably L1 or L2, but L3 is also fine. Once the states
 *  are large enough that all of them cannot fit inside L3 cache at the same
 *  time, performance takes a nose dive (4x loss minimum).
 */
function BloomFilter( array $t_args, array $input, array $output ) {
    grokit_assert( \count($output) == 1,
        'BloomFilter produces only 1 value, ' . \count($output) . ' outputs given.');

    $outputName = array_keys($output)[0];
    $outputType = array_get_index($output, 0);
    if( is_null($outputType) ) $outputType = lookupType('BASE::BIGINT');
    $output[$outputName] = $outputType;

    grokit_assert($outputType->is('numeric'), 'BloomFilter output must be numeric!');

    $exp = get_first_key_default($t_args, [ 'exponent' ], 16);
    grokit_assert(is_integer($exp), 'BloomFilter exponent must be an integer.');
    grokit_assert( $exp > 0 && $exp < 64, 'BloomFilter exponent must be in range (0,64), ' . $exp . ' given.');

    $nullCheck = get_default($t_args, 'null.check', false);
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
            grokit_assert(is_string($n), 'BloomFilster null.check has invalid value at position ' . $index);
            grokit_assert(array_key_exists($n, $nullable), 'BloomFilster null.check has unknown input ' . $n . ' at position ' . $index);

            $nullable[$n] = true;
        }
    } else {
        grokit_error('BloomFilster null.check must be boolean or list of inputs to check for nulls');
    }

    $debug = get_default($t_args, 'debug', 0);

    $bits = pow(2, $exp);
    $bytes = ceil($bits / 8.0);

    // Calculate the number of bits set for every possible value of a byte
    $nBits = [];
    for( $i = 0; $i < 256; $i++ ) {
        $n = $i;
        $b = 0;
        while( $n > 0 ) {
            $n &= ($n-1);
            $b++;
        }
        $nBits[$i] = $b;
    }

    $className = generate_name('BloomFilter');
?>
class <?=$className?> {
    static constexpr size_t BITS = <?=$bits?>;
    static constexpr size_t BYTES = <?=$bytes?>;
    static constexpr size_t MASK = BITS - 1;
    static constexpr std::array<unsigned char, 256> BITS_SET = { <?=implode(', ', $nBits)?> };
    static constexpr std::array<unsigned char, 8> BIT_MASKS = {
        0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80
    };

    size_t count;

    std::array<unsigned char, BYTES> set;
    //unsigned char set[BYTES];
    //std::bitset<BITS> set;

public:
    <?=$className?>() : count(0), set() {
        for( size_t i = 0; i < BYTES; i++ ) { //>
            set[i] = 0;
        }
    }

    ~<?=$className?>() { }

    void AddItem( <?=const_typed_ref_args($input)?> ) {
        count++;
<?  foreach( $nullable as $name => $check ) { ?>
<?      if( $check ) { ?>
        if( IsNull( <?=$name?> ) ) return;
<?      } // if checking for nulls ?>
<?  } // foreach input ?>
        size_t hashVal = H_b;
<?  foreach( $input as $name => $type ) { ?>
        hashVal = CongruentHash(Hash(<?=$name?>), hashVal);
<?  } // foreach input ?>
        hashVal = hashVal & MASK;
        const size_t bucket = hashVal >> 3;
        const size_t bucket_index = hashVal & 0x07;
        const unsigned char mask = BIT_MASKS[bucket_index];
        set[bucket] |= mask;
    }

    void AddState( <?=$className?> & o ) {
        count += o.count;
        for( size_t i = 0; i < BYTES; i++ ) { //>
            set[i] |= o.set[i];
        }
    }

    void GetResult( <?=$outputType?> & <?=$outputName?> ) {
        size_t nBitsSet = 0;
        constexpr long double bits = static_cast<long double>(BITS);
        for( size_t i = 0; i < BYTES; i++ ) { //>
            nBitsSet += BITS_SET[set[i]];
        }
        long double bitsSet = static_cast<long double>(nBitsSet);

        if( nBitsSet == BITS ) {
            // All Bits set, just give the cardinality as an estimate.
            <?=$outputName?> = count;
        } else {
            long double cardinality = - bits * std::log(1 - (bitsSet / bits));
            <?=$outputName?> = cardinality;
        }

<?  if( $debug > 0) { ?>
        std::cout << "BloomFilter:"
            << " bitsSet(" << bitsSet << ")"
            << " bits(" << bits << ")"
            << " cardinality(" << cardinality << ")"
            << " output(" << <?=$outputName?> << ")"
            << std::endl;; //>
<?  } // if debugging enabled ?>
    }
};

// Storage for static members
constexpr std::array<unsigned char, 256> <?=$className?>::BITS_SET;
constexpr std::array<unsigned char, 8> <?=$className?>::BIT_MASKS;
<?
    $system_headers = [ 'cmath', 'array' ];
    if( $debug > 0 ) {
        $system_headers[] = 'iostream';
    }

    return [
        'kind'          => 'GLA',
        'name'          => $className,
        'input'         => $input,
        'output'        => $output,
        'result_type'   => 'single',
        'user_headers'  => ['HashFunctions.h'],
        'system_headers'    => $system_headers,
    ];
}
?>
