<?php

declareFunction('DATE', ['BASE::DATETIME'], function($args) {
	$dateType = lookupType('BASE::DATE');
	$name = generate_name("DateTimeToDate");
?>

inline
DATE <?=$name?>( const DATETIME& dt ) {
	DATE d(dt.Year(), dt.Month(), dt.Day());
	return d;
}

<?
	return [
		'kind'		=> 'FUNCTION',
		'name'		=> $name,
		'input'		=> $args,
		'result'	=> $dateType,
		'deterministic' => true,
	];
});

?>