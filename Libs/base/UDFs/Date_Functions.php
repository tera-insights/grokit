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
      'kind'          => 'FUNCTION',
      'name'          => $name,
      'input'         => $args,
      'result'        => $dateType,
      'deterministic' => true,
  ];
});

// Constructs a DateTime from a Date and a Time.
declareFunction('DATETIME', ['BASE::DATE', 'BASE::TIME'], function($args) {
    $result = lookupType('BASE::DATETIME');
    $name = generate_name('DateTime')
?>

inline DATETIME <?=$name?>(const DATE& date, const TIME& time) {
  return DATETIME(date.GetYear(), date.GetMonth(), date.GetDay(),
                  time.hour(),    hour.minute(),   time.second());
}

<?
    return [
        'kind'          => 'FUNCTION',
        'name'          => $name,
        'input'         => $args,
        'result'        => $result,
        'deterministic' => true,
    ];
});

// Constructs a DateTime from a Date.
declareFunction('DATETIME', ['BASE::DATE'], function($args) {
    $result = lookupType('BASE::DATETIME');
    $name = generate_name('DateTime')
?>

inline DATETIME <?=$name?>(const DATE& date) {
  return DATETIME(date.GetYear(), date.GetMonth(), date.GetDay(), 0, 0, 0);
}

<?
    return [
        'kind'          => 'FUNCTION',
        'name'          => $name,
        'input'         => $args,
        'result'        => $result,
        'deterministic' => true,
    ];
});

// Constructs a Time from a DateTime.
declareFunction('TIME', ['BASE::DATETIME'], function($args) {
    $result = lookupType('BASE::TIME');
    $name = generate_name('Time');
?>

inline TIME <?=$name?>(const DATETIME& date_time) {
  return TIME(date_time.Hour(), date_time.Minute(), date_time.Second(), 0);
}

<?
    return [
        'kind'          => 'FUNCTION',
        'name'          => $name,
        'input'         => $args,
        'result'        => $result,
        'deterministic' => true,
    ];
});

?>

<?
declareFunction('DATETIMEFROMINT', ['BASE::INT'], function($args) {
    $result = lookupType('BASE::DATETIME');
    $name = generate_name('DateTimeFromInt');
?>

inline DATETIME <?=$name?>(const INT& seconds) {
  return DATETIME(seconds);
}

<?
    return [
        'kind'          => 'FUNCTION',
        'name'          => $name,
        'input'         => $args,
        'result'        => $result,
        'deterministic' => true,
    ];
});
?>
