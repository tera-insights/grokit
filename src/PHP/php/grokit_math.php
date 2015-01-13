<?php
// Copyright 2013 Tera Insights, LLC. All Rights Reserved.

namespace {

function gcd( $a, $b ) {
    $a = abs($a);
    $b = abs($b);

    if( $a == 0 and $b == 0 ) return 1; // special case for both zero
    if( $a == 0 ) return $b;
    if( $b == 0 ) return $a;
    if( $a == $b ) return $a;

    return $a > $b ? gcd($a - $b, $b) : gcd($a, $b - $a);
}

function lcm( $a, $b ) {
    return intval(abs($a) / gcd($a, $b)) * abs($b);
}

}

?>
