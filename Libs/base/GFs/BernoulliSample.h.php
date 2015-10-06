<?php

function BernoulliSampleState($t_args) {
	$sys_headers = $t_args['sys_headers'];
	$libs = $t_args['libs'];
	$ns = $t_args['namespace'];
	$rng = get_first_key_default($t_args, ['rng'], 'mt19937_64');

	$rng_full = "{$ns}::{$rng}";
	$rd_full = "{$ns}::random_device";

	$name = generate_name('BernoulliSampleState');
?>

class <?=$name?> {
public:
	typedef <?=$rng_full?> rng_type;
	typedef rng_type::result_type result_type;

private:

	typedef std::mutex mutex_t;
	typedef std::unique_lock<mutex_t> unique_lock_t;

	rng_type rng;
	mutex_t m_rng;

public:
	<?=$name?>() :
		rng(),
		m_rng()
	{
		<?=$rd_full?> rd;
		rng.seed(rd());
	}

	result_type operator() () {
		unique_lock_t rng_lock(m_rng);

		return rng();
	}
};

<?
	return [
		'kind'				=> 'RESOURCE',
		'name'				=> $name,
		'system_headers'	=> $sys_headers,
		'libraries'			=> $libs,
		'mutable'			=> true
	];
}

function BernoulliSample($t_args, $inputs) {
	$p = get_first_key_default($t_args, ['p', 0], 0.5);

	grokit_assert(is_float($p) || is_integer($p),
		"BernoulliSample: p must be a number in the range [0, 1]");
	grokit_assert($p >= 0 && $p <= 1,
		"BernoulliSample: p must be in the range [0, 1]");

	$rng = get_first_key_default($t_args, ['rng'], 'mt19937_64');
	$sys_headers = [ 'random' ]; // assuming std
	$libs = [ ]; // assuming std
	$ns = 'std'; // assuming standard library

	$cState = lookupResource('base::BernoulliSampleState',
		['sys_headers' => $sys_headers, 'libs' => $libs,
			'namespace' => $ns]);

	$name = generate_name('BernoulliSample');
?>

class <?=$name?> {
public:
	using state_type = <?=$cState?>;
	using rng_type = <?=$ns?>::<?=$rng?>;
	using dist_type = <?=$ns?>::bernoulli_distribution;

private:

	rng_type rng;
	dist_type bernoulli;

public:
	const constexpr static double P = <?=$p?>;

	<?=$name?>(state_type & _state):
		rng(_state()),
		bernoulli(P)
	{ }

	bool Filter( <?=const_typed_ref_args($inputs)?> ) {
		return bernoulli(rng);
	}
};

<?
	return [
		'kind'				=> 'GF',
		'name'				=> $name,
		'input'				=> $inputs,
		'system_headers' 	=> $sys_headers,
		'libraries'			=> $libs,
		'generated_state'	=> $cState,
	];
}

?>