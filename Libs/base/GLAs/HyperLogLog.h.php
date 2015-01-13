<?
// Copyright 2014 Tera Insights, LLC. All Rights Reserved.

/**
 *  A GLA that estimates the cardinality of a dataset using the HyperLogLog
 *  algorithm, with a configurable number of bins.
 */

function HyperLogLog( array $t_args, array $input, array $output ) {
    $debug = get_default($t_args, 'debug', 0);

    grokit_assert( \count($output) == 1,
        'HyperLogLog produces only 1 value, ' . \count($output) . ' outputs given.');

    $outputName = array_keys($output)[0];
    $outputType = array_get_index($output, 0);
    if( is_null($outputType) ) $outputType = lookupType('BASE::BIGINT');
    $output[$outputName] = $outputType;

    grokit_assert($outputType->is('numeric'), 'BloomFilter output must be numeric!');

    $exp = get_first_key_default($t_args, [ 'bins.exponent' ], 4 );
    grokit_assert(is_integer($exp), 'HyperLogLog bins.exponent must be an integer');

    // Set limit of 2^24 bins, because states past 16MB start to get silly
    grokit_assert( $exp >= 4 && $exp < 24, 'HyperLogLog bins.exponent must be in range [4, 24]');

    $useBuiltinCtz = get_default($t_args, 'use.builtin.ctz', true);
    $ctzFunc = $useBuiltinCtz ? '__builtin_ctzl' : 'ctz';

    $bins = pow(2, $exp);

    // Determine the value of alpha based on $exp
    switch($exp) {
    case 4:
        $alpha = 0.673;
        break;
    case 5:
        $alpha = 0.697;
        break;
    case 6:
        $alpha = 0.709;
        break;
    default:
        $alpha = 0.7213 / (1 + (1.079 / $bins));
    }

    $className = generate_name('HyperLogLog');
?>

class <?=$className?> {
    // Number of bins for registers
    static constexpr const size_t NUM_BINS = <?=$bins?>;
    // Number of bits used to index into registers, log2(NUM_BINS)
    static constexpr const size_t INDEX_BITS = <?=$exp?>;
    // Mask used to obtain register index from hash value
    static constexpr const size_t INDEX_MASK = NUM_BINS - 1;

    // Alpha coefficient used to correct cardinality estimate. Based on NUM_BINS.
    static constexpr const long double ALPHA = <?=$alpha?>;

    // Value of cardinality estimate after which we must apply the
    // large range correction
    static constexpr const long double LARGE_BREAKPOINT = (1.0 / 30.0) * <?=pow(2, 32)?>;

    // Constants for population count
    static constexpr const uint64_t m1  = 0x5555555555555555;
    static constexpr const uint64_t m2  = 0x3333333333333333;
    static constexpr const uint64_t m4  = 0x0f0f0f0f0f0f0f0f;
    static constexpr const uint64_t h01 = 0x0101010101010101;

    // The registers
    std::array<unsigned char, NUM_BINS> registers;

    // A count used to remember how many tuples were processed, mostly for debugging.
    size_t count;

public:

    <?=$className?>(void) : registers() {
        for( auto & elem : registers ) {
            elem = 0;
        }
    }

    ~<?=$className?>() { }

    int popcount(uint64_t x) {
        // Put count of each 2 bits into those 2 bits
        x -= (x >> 1) & m1;
        // Put count of each 4 bits into those 4 bits
        x = (x & m2) + ((x >> 2) & m2);
        // Put count of each 8 bits into those 8 bits
        x  = (x + (x >> 4)) & m4;
        // Returns left 8 bits of x + (x << 8) + (x << 16) + ...
        return (x * h01) >> 56;
    }

    int ctz(int64_t x) {
        return popcount((x & -x) - 1);
    }

    void AddItem( <?=const_typed_ref_args($input)?> ) {
        count++;

        uint64_t hashVal = H_b;
<?  foreach($input as $name => $type) { ?>
        hashVal = CongruentHash(Hash(<?=$name?>), hashVal);
<?  } // for each input ?>

        const size_t registerIndex = hashVal & INDEX_MASK;
        uint64_t value = hashVal >> INDEX_BITS;
        unsigned char nZeros = <?=$ctzFunc?>(value);

        unsigned char & registerValue = registers[registerIndex];
        registerValue = registerValue > nZeros ? registerValue : nZeros;
    }

    void AddState( <?=$className?> & other ) {
        for( size_t i = 0; NUM_BINS > i; i++ ) {
            unsigned char & rVal = registers[i];
            unsigned char & oVal = other.registers[i];
            rVal = rVal > oVal ? rVal : oVal;
        }
    }

    void GetResult( <?=$outputType?> & <?=$outputName?> ) {
        // Compute harmonic sum of registers and correct by alpha
        long double cardEst = 0;
        size_t nZeroRegisters = 0;
        for( auto elem : registers ) {
            long double power = - static_cast<long double>(elem);
            cardEst += std::pow(2.0, power);

            if( elem == 0 )
                nZeroRegisters++;
        }
        const long double nBins = static_cast<long double>(NUM_BINS);
        const long double zeroBins = static_cast<long double>(nZeroRegisters);

        cardEst = 1 / cardEst;
        cardEst *= ALPHA * nBins * nBins;

        long double cardinality = cardEst;

        if( (cardEst < 2.5 * NUM_BINS) ) { //>
            // Possible small range correction
            if( nZeroRegisters > 0 ) {
                // Small range correction
                cardinality = nBins * std::log(nBins / zeroBins);
            }
        }
        // TODO: Figure out if the large range correction is needed for 64-bit
        // hashes.

        <?=$outputName?> = cardinality;
    }
};
<?
    $system_headers = [ 'cmath', 'array', 'cinttypes' ];
    if( $debug > 0 ) {
        $system_headers[] = 'iostream';
    }

    return [
        'kind'      => 'GLA',
        'name'      => $className,
        'input'     => $input,
        'output'    => $output,
        'result_type'   => 'single',
        'user_headers' => [ 'HashFunctions.h' ],
        'system_headers' => $system_headers,
    ];
}
?>
