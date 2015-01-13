<?
// Copyright 2013 Tera Insights, LLC. All Rights Reserved.
//
// Author: Christopher Dudley

function InverseGaussianGen( array $t_args ) {
    grokit_assert(array_key_exists('output', $t_args), 'No output type specified for inverse gaussian generator.');
    grokit_assert(array_key_exists('ns', $t_args), 'No namespace specified for inverse gaussian generator.');

    $output = $t_args['output'];
    $ns = $t_args['ns'];

    $norm = $ns . '::' . 'normal_distribution<double>';
    $uni = $ns . '::' . 'uniform_real_distribution<double>';

    $className = generate_name('InverseGaussianGen');
?>

class <?=$className?> {
    <?=$norm?> norm;
    <?=$uni?> uni;

    typedef long double real_t;

    const real_t mean;
    const real_t shape;
    const real_t mean_2;
    const real_t mean_o_2_shape;

public:
    <?=$className?>(void) = delete;

    <?=$className?>( <?=$output?> _mean, <?=$output?> _shape ):
        norm(0.0, 1.0),
        uni(0.0, 1.0),
        mean(_mean),
        shape(_shape),
        mean_2(_mean * _mean),
        mean_o_2_shape(_mean / (2 * _shape))
    { }

    template< class Generator >
    <?=$output?> operator() ( Generator & g ) {
        const real_t v = norm(g);
        const real_t y = v * v;
        const real_t y_2 = y * y;

        const real_t x = (mean) + (mean_o_2_shape * mean * y) - (mean_o_2_shape * std::sqrt((4 * mean * shape * y) + (mean_2 * y_2)));

        const real_t z = uni(g);

        const real_t deci = mean / (mean + x);

        return z > deci ? (mean_2 / x) : x;
    }
};

<?
    return array(
        'kind'      => 'RESOURCE',
        'name'      => $className,
        'system_headers' => [ 'cmath' ],
    );
} // end InverseGaussianGen

/**
 *  GI that generates data in clusters, using a specified distribution for each
 *  cluster.
 *
 *  This GI requires the following template arguments:
 *      - 'n' or 0
 *          The number of tuples to generate. Note: this value is per task.
 *          The total number of tuples generated will be n_tasks * n
 *      - 'centers' or 1
 *          A list of configuration for the centers.
 *
 *  The following template arguments are optional:
 *      - 'outputs'
 *          If the outputs of the GI are not given implicitly, they can be
 *          specified in this template argument. The number of dimensions will
 *          be determined by the number of outputs.
 *
 *          All output types must be numeric real types. The default type for
 *          outputs is DOUBLE.
 *      - 'dist.lib' = 'std'
 *          Which library to use for generating distributions.
 *          Valid options are:
 *              - std
 *              - boost
 *      - 'seed' = null
 *          The seed to be used for the random number generator. This seed will
 *          be used to generate the seed for each task, and different runs with
 *          the same seed will produce the same data.
 *      - 'compute.sets' = 1
 *          The number of sets of tuples to compute at once.
 *
 *  Each center configuration is a functor with the form:
 *      dist_name(args)
 *
 *  The following distributions are supported:
 *      { Uniform Distributions }
 *      - uniform(a = 0, b = 1)
 *
 *      { Normal Distributions }
 *      - normal(mean = 0.0, std_dev = 1.0) [ synonyms: gaussian ]
 *      - inverse_gaussian(mean = 1, shape = 1) [ synonyms: inverse_normal ]
 *
 *      { Bernoulli Distributions }
 *      - binomial(t = 1, p = 0.5)
 *      - negative_binomial(k = 1, p = 0.5)
 *
 *      { Poisson Distributions }
 *      - exponential( lambda = 1 )
 *      - gamma(alpha = 1, beta = 1)    [ synonyms: Gamma ]
 */
function ClusterGen( array $t_args, array $outputs ) {
    $sys_headers = [ 'array', 'cinttypes' ];
    $user_headers = [ ];
    $libraries = [];

    if( \count($outputs) == 0 ) {
        grokit_assert(array_key_exists('outputs', $t_args),
            'ClusterGen: No outputs specified');

        $count = 0;
        foreach( $t_args['outputs'] as $type ) {
            if( is_identifier($type) )
                $type = lookupType($type);

            grokit_assert(is_datatype($type),
                'ClusterGen: Non data-type ' . $type . ' given as output');

            $name = 'output' . $count++;
            $outputs[$name] = $type;
        }
    }

    foreach( $outputs as $name => &$type ) {
        if( is_null($type) ) {
            $type = lookupType('base::DOUBLE');
        }
        else {
            grokit_assert($type->is('real'),
                'ClusterGen: Non-real datatype ' . $type . ' given as output');
        }
    }

    $myOutputs = [];
    foreach( $outputs as $name => $type ) {
        $myOutputs[$name] = $type;
    }

    $tSize = \count($outputs);

    $seed = get_default($t_args, 'seed', null);
    if( $seed !== null ) {
        grokit_assert(is_int($seed), 'ClusterGen: Seed must be an integer or null.');
    } else {
        $user_headers[] = 'HashFunctions.h';
    }

    $distLib = get_default($t_args, 'dist.lib', 'std');
    $distNS = '';
    switch( $distLib ) {
    case 'std':
        $sys_headers[] = 'random';
        $distNS = 'std';
        break;
    case 'boost':
        $sys_headers[] = 'boost/random.hpp';
        $distNS = 'boost::random';
        $libraries[] = 'boost_random-mt';

        if( $seed === null ) {
            // Need random_device
            $sys_headers[] = 'boost/random/random_device.hpp';
            $libraries[] = 'boost_system-mt';
        }
        break;
    default:
        grokit_error('ClusterGen: Unknown RNG library ' . $distLib);
    }

    $distRNG = 'mt19937';
    $RNGtype = $distNS . '::' . $distRNG;

    $nTuples = get_first_key($t_args, ['n', '0']);
    grokit_assert(is_int($nTuples),
        'ClusterGen: the number of tuples to be produced must be an integer.');

    $centers = get_first_key($t_args, [ 'centers', 1 ] );
    grokit_assert(is_array($centers),
        'ClusterGen: centers must be an array of functors');

    $handleDist = function($name, $args, $oType) use ($distNS) {
        $distName = '';
        $distArgs = [];

        switch( $name ) {
        case 'gaussian':
        case 'normal':
            $distName = $distNS . '::' . 'normal_distribution<' . $oType . '>';
            grokit_assert(\count($args) <= 2,
                'ClusterGen: Normal distribution takes at most 2 arguments, ' . \count($args) . ' given');

            $mean = get_default($args, ['mean', 0], 0.0);
            $sigma = get_default($args, ['std_dev', 'sigma', 1], 1.0);

            grokit_assert( is_numeric($mean), 'ClusterGen: mean parameter of binomial distribution must be a real number.');
            grokit_assert( is_numeric($sigma), 'ClusterGen: sigma parameter of binomial distribution must be a real number.');
            $mean = floatval($mean);
            $sigma = floatval($sigma);

            $distArgs = [ $mean, $sigma ];
            break;
        case 'binomial':
            $distName = $distNS . '::' . 'binomial_distribution<' . $oType . '>';
            grokit_assert(\count($args) <= 2,
                'ClusterGen: Binomial distribution takes at most 2 arguments, ' . \count($args) . ' given');

            $t = get_default($args, ['t', 0], 1);
            $p = get_default($args, ['p', 1], 0.5);

            grokit_assert( is_int($t), 'ClusterGen: t parameter of binomial distribution must be an integer.');
            grokit_assert( is_numeric($p), 'ClusterGen: p parameter of binomial distribution must be a real number.');
            $p = floatval($p);

            grokit_assert( $p >= 0 && $p <= 1, 'ClusterGen: p parameter of binomial distribution must be in the range [0, 1]');
            grokit_assert( $t >= 0, 'ClusterGen: t parameter of binomial distribution must be in the range [0, +inf)');

            $distArgs = [ $t, $p ];
            break;
        case 'negative_binomial':
            $distName = $distNS . '::' . 'negative_binomial_distribution<' . $oType . '>';
            grokit_assert(\count($args) <= 2,
                'ClusterGen: Negative Binomial distribution takes at most 2 arguments, ' . \count($args) . ' given');

            $k = get_default($args, ['k', 0], 1);
            $p = get_default($args, ['p', 1], 0.5);

            grokit_assert( is_int($k), 'ClusterGen: k parameter of binomial distribution must be an integer.');
            grokit_assert( is_numeric($p), 'ClusterGen: p parameter of binomial distribution must be a real number.');
            $p = floatval($p);

            grokit_assert( $p > 0 && $p <= 1, 'ClusterGen: p parameter of negative binomial distribution must be in the range (0, 1]');
            grokit_assert( $k > 0, 'ClusterGen: k parameter of negative binomial distribution must be in the range (0, +inf)');

            $distArgs = [ $k, $p ];
            break;
        case 'inverse_gaussian':
        case 'inverse_normal':
            grokit_assert(\count($args) <= 2,
                'ClusterGen: Inverse Gaussian distribution takes at most 2 arguments, ' . \count($args) . ' given');

            $mean = get_default($args, ['mean', 0], 1);
            $shape = get_default($args, ['shape', 1], 1);

            grokit_assert( is_numeric($mean), 'ClusterGen: mean parameter of inverse gaussian distribution must be a real number.');
            grokit_assert( is_numeric($shape), 'ClusterGen: shape parameter of inverse gaussian distribution must be a real number.');
            $mean = floatval($mean);
            $shape = floatval($shape);

            grokit_assert( $mean > 0, 'ClusterGen: mean of inverse gaussian distribution must be in range (0, inf)');
            grokit_assert( $shape > 0, 'ClusterGen: shape of inverse gaussian distribution must be in range (0, inf)');

            $gen_args = [ 'output' => $oType, 'ns' => $distNS ];
            $distName = strval(lookupResource('datagen::InverseGaussianGen', $gen_args));
            $distArgs = [ $mean, $shape ];

            break;
        case 'uniform':
            $distName = $distNS . '::' . 'uniform_real_distribution<' . $oType . '>';
            grokit_assert(\count($args) <= 2,
                'ClusterGen: Uniform distribution takes at most 2 arguments, ' . \count($args) . ' given');

            $a = get_default($args, ['a', 0], 0.0);
            $b = get_default($args, ['b', 1], 1.0);

            grokit_assert( is_numeric($a), 'ClusterGen: `a` parameter of uniform distribution must be a real number.');
            grokit_assert( is_numeric($b), 'ClusterGen: `b` parameter of uniform distribution must be a real number.');
            $a = floatval($a);
            $b = floatval($b);

            grokit_assert( $b >= $a, 'ClusterGen: `b` parameter of uniform distribution must be >= the `a` parameter.');

            $distArgs = [ $a, $b ];
            break;
        case 'exponential':
            $distName = $distNS . '::' . 'exponential_distribution<'. $oType . '>';
            grokit_assert(\count($args) <= 1,
                'ClusterGen: Exponential distribution takes at most 1 argument.');

            $lambda = get_default($args, [ 'lambda', 0 ], 1.0);
            grokit_assert( is_numeric($lambda), 'ClusterGen: `lambda` parameter of exponential distribution must be a real number.');
            $lambda = floatval($lambda);

            grokit_assert( $lambda > 0, 'ClusterGen: `lambda` parameter of exponential distribution must be in range (0, +inf).');

            $distArgs = [ $lambda ];
            break;
        case 'gamma':
        case 'Gamma':
            $distName = $distNS . '::' . 'gamma_distribution<'. $oType . '>';
            grokit_assert(\count($args) <= 2,
                'ClusterGen: Gamma distribution takes at most 2 arguments.');

            $alpha = get_default($args, ['alpha', 0], 1.0);
            $beta = det_default($args, ['beta', 1], 1.0);
            grokit_assert( is_numeric($alpha), 'ClusterGen: `alpha` parameter of gamma distribution must be a real number.');
            grokit_assert( is_numeric($beta), 'ClusterGen: `beta` parameter of gamma distribution must be a real number.');
            $alpha = floatval($alpha);
            $beta = floatval($beta);

            $distArgs = [ $alpha, $beta ];

            break;
        default:
            grokit_error('ClusterGen: Unknown distribution ' . $name . ' given for center');
        }

        return [ $distName, $distArgs ];
    };

    $dists = [];
    $distArgs = [];
    $count = 0;
    $oType = '';
    $nCenters = 1;
    reset($outputs);
    foreach( $centers as $val ) {
        $cluster = $val;
        if( is_functor($val) ) {
            $cluster = [ $val ];
        }
        else if( is_array( $val ) ) {
            $nCenters = lcm($nCenters, \count($val));
        }
        else {
            grokit_error('ClusterGen: center descriptions must be functors or list of functors');
        }

        $curDist = [];
        $curDistArgs = [];
        $curDistName = 'distribution' . $count++;

        $oType = strval(current($outputs));
        $iCount = 0;
        foreach ( $cluster as $functor ) {
            grokit_assert(is_functor($functor), 'ClusterGen: center description must be a functor');

            $vName = $curDistName . '_' . $iCount++;
            $ret = $handleDist($functor->name(), $functor->args(), $oType);
            $curDist[$vName] = $ret[0];
            $curDistArgs[$vName] = $ret[1];
        }

        next($outputs);

        $dists[$curDistName] = $curDist;
        $distArgs[$curDistName] = $curDistArgs;
    }

    // Determine the default number of sets to compute at a time.
    // We want to generate either $nTuples or 10,000 tuples, depending on which
    // is less.
    $defaultSetsTarget = min($nTuples, 10000);
    $setsToTarget = intval(ceil($defaultSetsTarget / $nCenters));

    $computeSets = get_default( $t_args, 'compute.sets', $setsToTarget );
    grokit_assert(is_int($computeSets) && $computeSets > 0,
        'ClusterGen: compute.sets must be a positive integer, ' . $computeSets . ' given');

    $className = generate_name('ClusterGen');

    // For some BIZZARE reason, the $outputs array was getting modified while
    // traversing over the $dists array. Making a deep copy of the outputs and
    // then reassigning it seems to fix the issue.
    $outputs = $myOutputs;

?>

class <?=$className?> {

    // The number of tuples to produce per task
    static constexpr size_t N = <?=$nTuples?>;
    static constexpr size_t CacheSize = <?=$computeSets * $nCenters?>;

    // Typedefs
    typedef std::tuple<<?=array_template('{val}', ', ', $outputs)?>> Tuple;
    typedef std::array<Tuple, CacheSize> TupleArray;
    typedef TupleArray::const_iterator TupleIterator;
    typedef <?=$RNGtype?> RandGen;

    // Number of tuples produced.
    uintmax_t count;

    // Cache a number of outputs for efficiency
    TupleArray cache;
    TupleIterator cacheIt;

    // Random number generator
    RandGen rng;

    // Distributions
<?  // This is the section causing issues.
    foreach($dists as $name => $list)  {
        foreach( $list as $vName => $type ) {
?>
    <?=$type?> <?=$vName?>;
<?
        } // foreach distribution
    } // foreach cluster set
?>

    // Helper function to generate tuples.
    void GenerateTuples(void) {
<?
    $tIndex = 0;
    foreach($dists as $name => $list)  {
        $lCenters = \count($list);
        // $nCenters has been defined to be the LCM of the number of centers in
        // any column, so $lCenter is guaranteed to divide evenly into
        // CacheSize
?>
        for( size_t index = 0; CacheSize > index; index += <?=$lCenters?> ) {
<?
        $index = 0;
        foreach( $list as $vName => $type  ) {
?>
            std::get<<?=$tIndex?>>(cache[index + <?=$index?>]) = <?=$vName?>(rng);
<?
            $index++;
        } // foreach value in tuple
?>
        }
<?
        $tIndex++;
    } // foreach distribution
?>
        cacheIt = cache.cbegin();
    }

public:
    // Constructor
    <?=$className?>( GIStreamProxy & _stream ) :
        cache()
        , cacheIt()
        , count(0)
        , rng()
<?  foreach($dists as $name => $list)  {
        foreach( $list as $vName => $type ) {
?>
        , <?=$vName?>(<?=implode(', ', $distArgs[$name][$vName])?>)
<?
        } // foreach distribution
    } // foreach cluster set
?>

    {
<?  if( is_null($seed) ) { ?>
        <?=$distNS?>::random_device rd;
<?  } // if seed is null ?>
        RandGen::result_type seed = <?=is_null($seed) ? 'rd()' : "CongruentHash($seed, _stream.get_id() )"?>;
        rng.seed(seed);

        cacheIt = cache.cend();
    }

    // Destructor
    ~<?=$className?>(void) { }

    bool ProduceTuple(<?=typed_ref_args($outputs)?>) {
        if( N > count ) {
            if( cacheIt == cache.cend() ) {
                GenerateTuples();
            }
<?
    $tIndex = 0;
    foreach($outputs as $name => $type) {
?>
            <?=$name?> = std::get<<?=$tIndex?>>(*cacheIt);
<?
        $tIndex++;
    } // foreach output
?>

            ++cacheIt;
            ++count;

            return true;
        }
        else {
            return false;
        }
    }
};

<?

    return array(
        'kind'           => 'GI',
        'name'           => $className,
        'output'         => $outputs,
        'system_headers' => $sys_headers,
        'user_headers'   => $user_headers,
        'libraries'      => $libraries,
    );
}
?>
