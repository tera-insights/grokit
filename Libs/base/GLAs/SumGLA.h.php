<?
// Copyright 2013 Tera Insights, LLC. All Rights Reserved

function Sum( array $t_args, array $inputs, array $outputs ) {

    $className = generate_name("Sum");

    $storage = [];
    $inits = [];
    if( \count($inputs) == 0 ) {
        $inputs = [ "x" => lookupType("base::DOUBLE") ];
        $storage = [ "x" => 'long double' ];
        $inits = [ "x" => '' ];
        $outputs = $inputs;
    }
    else {
        $oInputs = $inputs;
        reset($outputs);
        foreach( $oInputs as $name => $value ) {
            if( $value->is('real') ) {
                $storage[$name] = 'long double';
            } else if( $value->is('integral') ) {
                $storage[$name] = 'long long int';
            } else {
                $storage[$name] = $value->value();
            }
            $oKey = key($outputs);

            if( $outputs[$oKey] === null ) {
                if( $value->is('real') ) {
                    $outputs[$oKey] = lookupType('base::DOUBLE');
                } else if ($value->is('integral')) {
                    $outputs[$oKey] = lookupType('base::BIGINT');
                } else {
                    $outputs[$oKey] = $value;
                }
            }

            $inits[$name] = $value->has('init') ? $value->get('init') : '';

            next($outputs);
        }
    }
?>
class <?=$className?> {
    <?=array_template('{val} {key};' . PHP_EOL, '    ', $storage)?>

public:
    <?=$className?>() : <?=array_template('{key}({val})', ', ', $inits) ?>
    { }

    void AddItem(<?=array_template('const {val}& _{key}', ', ', $inputs)?>) {
        <?=array_template('{key} += _{key};' . PHP_EOL, '        ', $inputs)?>
    }

    void AddState( <?=$className?> & other ) {
        <?=array_template('{key} += other.{key};' . PHP_EOL, '        ', $inputs)?>
    }

    void GetResult( <?=array_template('{val}& _{key}', ', ', $outputs)?> ) {
<?
    reset($outputs);
    reset($inputs);
    foreach( $outputs as $name => $type ) {
        $inName = key($inputs);
?>
        _<?=$name?> = <?=$inName?>;
<?

        next($inputs);
    }
?>
    }
};

<?
  return  array (
      'kind'        => 'GLA',
      'name'        => $className,
      'input'       => $inputs,
      'output'      => $outputs,
      'result_type' => 'single',
  );

}
?>
