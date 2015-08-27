<?php
// Copyright 2015, Tera Insights, LLC. All Rights Reserved.

declareFunctionGlobal('BASE', 'BOOL', ['BASE::NULL'], function() {
	$boolType = lookupType('BASE::BOOL');
	$nullType = lookupType('BASE::NULL');

	return [
		'kind' 		=> 'FUNCTION',
		'name'		=> 'bool_Null',
		'input'		=> [$nullType],
		'result'	=> $boolType,
		'deterministic' => true
	];
});
?>

inline int ToString(bool b, char * buffer) {
	if (b)
		return 1 + sprintf(buffer, "true");
	else
		return 1 + sprintf(buffer, "false");
}

inline void FromString(bool & b, const char* buffer) {
	char first = *buffer;
	char upper = *buffer & 0xDF;
	if (first == '1' || upper == 'T' || upper == 'Y')
		b = true;
	else
		b = false;
}

namespace BASE {
	inline
	bool bool_Null(const GrokitNull& null) {
		return false;
	}
}
