<?
// Copyright 2013 Tera Insights, LLC. All Rights Reserved

function GroupByState(array $t_args) {
    $className = generate_name('GroupByState_');
    $sys_headers = [ ];

    $gla = $t_args['gla'];
    $gbyAtts = $t_args['groups'];
    $debug = get_default($t_args, 'debug', 0);

    $cArgs = [];

    $configurable = $gla->has_state() && $gla->state()->configurable();

    if( $configurable ) {
        $cArgs['json'] = 'Json::Value';
    }

    $reqStates = $gla->req_states();
    $hasReqStates = \count($reqStates) > 0;
    if( $hasReqStates ) {
        foreach( $reqStates as $name => $type ) {
            $cArgs[$name] = $type;
        }
    }

    $nCArgs = \count($cArgs);
    if( $nCArgs > 0 ) {
        $cstStr = '(' . implode(', ', array_keys($cArgs)) . ')';
    } else {
        $cstStr = '';
    }

    $iterable = $gla->iterable();

    if( $iterable ) {
        $sys_headers[] = 'unordered_map';
        $mapType = 'std::unordered_map<Key, InnerState, HashKey>';
    }

    if( $debug > 0 ) {
        $sys_headers[] = 'cstdio';
        $sys_headers[] = 'string';
    }
?>
struct <?=$className?> {
    struct Key {
        <?=array_template("{val} {key};", PHP_EOL , $gbyAtts)?>

        Key () : <?=array_template('{key}()', ', ', $gbyAtts)?> {}

        Key (<?=array_template('const {val} & _{key}', ', ', $gbyAtts)?>) :
            <?=array_template('{key}(_{key})', ',' . PHP_EOL, $gbyAtts)?>
        { }

        bool operator==(const Key& o) const {
            return ( <?=array_template('{key} == o.{key}', ' && ', $gbyAtts)?> );
        }

        size_t hash_value() const {
            uint64_t hash= H_b;
        <?=array_template( "hash = CongruentHash(Hash({key}), hash);", PHP_EOL, $gbyAtts)?>

            return (size_t) hash;
        }

<?  foreach (array_keys($gbyAtts) as $index => $key) { ?>
        const <?=$gbyAtts[$key]?>& GetKey<?=$index?>() const {
            return <?=$key?>;
        }
<?  } ?>

<?  if( $debug > 0 ) { ?>
        std::string to_string() const {
            char buffer[<?=pow(2, 12)?>];
            size_t pos = 0;

            buffer[pos++] = '(';
<?      foreach( $gbyAtts as $name => $type ) { ?>
            pos += ToString(<?=$name?>, buffer+pos);
            buffer[pos-1] = ',';
<?      } // for each grouping attribute ?>
            buffer[pos-1] = ')';
            buffer[pos++] = '\0';

            return std::string(buffer);
        }
<?  } // if debugging enabled ?>
    };

    struct HashKey {
        size_t operator()(const Key& o) const {
            return o.hash_value();
        }
    };

<?  if( $gla->has_state() ) { ?>
    using InnerState = <?=$gla->state()?>;
    const InnerState initialState;

<?      if( $iterable ) { ?>
    using StateMap = <?=$mapType?>;
    StateMap stateMap;
<?      } // if iterable ?>

    <?=$className?>(<?=const_typed_ref_args($cArgs)?>) :
        initialState(<?=args($cArgs)?>)
<?      if( $iterable ) { ?>
        , stateMap()
<?      } // if iterable ?>
    { }

    ~<?=$className?>() { }

    const InnerState & getConstState(const Key & key) const {
<?      if( $iterable ) { ?>
        StateMap::const_iterator it = stateMap.find(key);
        if( it != stateMap.cend() ) {
            return it->second;
        }
<?          if( $debug > 0 ) { ?>
        std::string keyStr = key.to_string();
        fprintf(stderr, "[CONST] State not found for key %s, returning initial state.\n", keyStr.c_str());
<?          } // if debugging enabled ?>
<?      } // if iterable ?>
        return initialState;
    }

<?      if( $iterable ) { ?>
    InnerState & getModibleState(const Key & key) {
        StateMap::iterator it = stateMap.find(key);
        if( it == stateMap.end() ) {
<?          if( $debug > 0 ) { ?>
            std::string keyStr = key.to_string();
            fprintf(stderr, "[MOD]   State not found for key %s, generating new state.\n", keyStr.c_str());
<?          } // if debugging enabled ?>
            InnerState nb(initialState);

            stateMap.insert(StateMap::value_type(key, nb));
            it = stateMap.find(key);
        }

        return it->second;
    }
<?      } // if iterable ?>
<?  } // if gla has state ?>
};
<?
    return [
        'kind'          => 'RESOURCE',
        'name'          => $className,
        'system_headers'    => $sys_headers,
        'configurable'  => $configurable,
    ];
}

/*
 *  Template Arguments:
 *
 *      [R] 'group':        A list of string corresponding to the names of input expressions
 *                          that are to be used as grouping attributes.
 *      [R] 'aggregate':    A GLA to be used as the aggregate.
 *
 *  Any expressions used as grouping attributes must be named.
 */
function GroupBy( array $t_args, array $inputs, array $outputs, array $states ) {

    // Ensure we have valid inputs.
    if( \count($inputs) == 0 ) {
        // No inputs given, try to get them from template arguments.
        grokit_assert(array_key_exists('input', $t_args), 'No inputs given for GroupBy');
        $inputs = $t_args['input'];
        if( !is_array($inputs) ) $inputs = [ $inputs ];

        foreach( $inputs as $name => &$type ) {
            if( is_identifier($type) ) {
                $type = lookupType(strval($type));
            }

            grokit_assert( is_datatype($type), 'Invalid type given for input ' . $name );
        }
    }

    grokit_assert( array_key_exists('group', $t_args), 'No groups specified for GroupBy');
    $gbyAttMap = $t_args['group'];

    grokit_assert( is_array($gbyAttMap), 'Invalid value given for groups, expected an expression name or list of expression names');
    $gbyAttMap = array_map('strval', $gbyAttMap);
    $gbyAttNames = array_keys($gbyAttMap);
    foreach( $gbyAttMap as $in => $out ) {
        grokit_assert( array_key_exists($in, $inputs), 'Group ' . $in . ' not present in input');
        grokit_assert( array_key_exists($out, $outputs), 'Output Attribute ' . $out . ' for group ' . $in . ' not found in outputs');
    }

    $numGByAtts = \count($gbyAttNames);

    grokit_assert( array_key_exists('aggregate', $t_args), 'No aggregate specified for GroupBy' );
    $innerGLA = $t_args['aggregate'];
    grokit_assert( is_gla($innerGLA), 'Non-GLA specified as aggregate for GroupBy');

    $debug = get_default( $t_args, 'debug', 0);

    $init_size = get_default( $t_args, 'init.size', 1024);
    $use_mct = get_default( $t_args, 'use.mct', true);
    $keepHashes = get_default($t_args, 'mct.keep.hashes', false);
    grokit_assert(is_bool($keepHashes), 'GroupBy mct.keep.hashes argument must be boolean');

    // determine the result type
    $use_fragments = get_default( $t_args, 'use.fragments', true);
    $resType = $use_fragments ? [ 'fragment', 'multi' ] : [ 'multi' ];
    $fragSize = get_default($t_args, 'fragment.size', 2000000);

    // Always support state
    $resType[] = 'state';

    // Class name randomly generated
    $className = generate_name("GroupBy");

    // instantiate the inner GLA. input/output is derived from the main input/output
    $gbyAtts = [];
    $gbyAttsOut = [];
    $glaInputAtts = [];
    $glaOutputAtts = [];
    foreach( $inputs as $name => $type ) {
        if( in_array($name, $gbyAttNames) ) {
            $gbyAtts[$name] = $type;
            $gbyAttsOut[$gbyAttMap[$name]] = $type;
            $outputs[$gbyAttMap[$name]] = $type;
        }
        else {
            $glaInputAtts[$name] = $type;
        }
    }
    foreach( $outputs as $name => $type ) {
        if( !in_array($name, $gbyAttMap) ) {
            $glaOutputAtts[$name] = $type;
        }
    }

    $innerGLA = $innerGLA->apply($glaInputAtts, $glaOutputAtts, $states);
    $libraries = $innerGLA->libraries();

    $innerRes = get_first_value( $innerGLA->result_type(), [ 'multi', 'single', 'state' ] );

    if( $innerRes == 'state' ) {
        // If the result type is state, the only output is a state object
        // containing the GLA.
        $outputName = array_keys($glaOutputAtts)[0];
        $innerOutputs = [ $outputName => lookupType('base::STATE', [ 'type' => $innerGLA ] ) ];
    }
    else {
        $innerOutputs = $innerGLA->output();

        grokit_assert(\count($innerOutputs) == \count($glaOutputAtts),
            'Expected ' . \count($glaOutputAtts) . ' outputs fromm Inner GLA, got ' .
            \count($innerOutputs));
    }

    $constState = lookupResource('GroupByState',
        [ 'gla' => $innerGLA, 'groups' => $gbyAtts, 'debug' => $debug ]);

    // constructor argumetns are inherited from inner GLA
    $configurable = $innerGLA->configurable();
    $reqStates = $innerGLA->req_states();

    // We need to specially create the constructor string because apparently
    // declaring Type Name(); is a function declaration instead of a variable
    // declaration for some reason.
    $constructorParts = [];
    if( $configurable ) {
        $constructorParts[] = 'jsonInit';
    }
    if( $innerGLA->has_state() ) {
        $constructorParts[] = 'innerState';
    }

    $constructorString = \count($constructorParts) > 0 ? '(' . implode(', ', $constructorParts) . ')' : '';

    // add the outputs we got from the gla
    foreach( $innerOutputs as $name => $type ) {
        grokit_assert( array_key_exists($name, $outputs), 'Inner GLA\'s outputs refer to unknown attribute ' . $name );
        grokit_assert($type !== null, 'GroupBy Inner GLA left output ' . $name . ' with no type');
        $outputs[$name] = $type;
    }

    $iterable = $innerGLA->iterable();

    // need to keep track of system includes needed
    $extraHeaders = array();

    $allocatorText = "std::allocator<std::pair<const Key, {$innerGLA}> >";
    if($use_mct) {
        $keepHashesText = $keepHashes ? 'true' : 'false';
        $extraHeaders[] = "mct/hash-map.hpp";
        $map = "mct::closed_hash_map<Key, {$innerGLA}, HashKey, std::equal_to<Key>, {$allocatorText}, {$keepHashesText}>";
        $mapType = 'mct::closed_hash_map';
    } else {
        $extraHeaders[] = "unordered_map";
        $map = "std::unordered_map<Key, {$innerGLA}, HashKey, std::equal_to<Key>, {$allocatorText}>";
        $mapType = 'std::unordered_map';
    }

    if( $debug > 0 ) {
        $extraHeaders[] = 'cstdio';
    }
?>


class <?=$className?>{
public:
    using ConstantState = <?=$constState?>;
<?  if( $innerGLA->has_state() ) { ?>
    using InnerState = ConstantState::InnerState;
<?  } // if gla has state ?>
    using Key = ConstantState::Key;
    using HashKey = ConstantState::HashKey;
    using InnerGLA = <?=$innerGLA?>;

    typedef <?=$map?> MapType;
    static const size_t INIT_SIZE = <?=$init_size?>;

public:
    class Iterator {
        MapType::iterator it; // current value
        MapType::iterator end; // last value in the fragment

    public:
        Iterator() { }

        Iterator(MapType::iterator _it, MapType::iterator _end):
            it(_it), end(_end)
        {
            if( it != end ) {

<?
        switch( $innerRes ) {
        case 'multi':
?>
            it->second.Finalize();
<?
            break;
        case 'state':
            if( $innerGLA->finalize_as_state() ) {
?>
            it->second.FinalizeState();
<?
            } // if we need to finalize as a state
            break;
        } // end switch inner restype
?>
            }
        }

        bool GetNextResult( <?=typed_ref_args($outputs)?> ) {
            bool gotResult = false;
            while( it != end && !gotResult ) {
                <?=$innerGLA?> & gla = it->second;
<?  foreach( $gbyAttMap as $in => $out ) { ?>
                <?=$out?> = it->first.<?=$in?>;
<?  } // foreach grouping attribute ?>

<?      switch( $innerRes ) {
            case 'multi': ?>
                gotResult = gla.GetNextResult( <?=args($innerOutputs)?>);
                if( !gotResult ) {
                    ++it;
                    if( it != end ) {
                        it->second.Finalize();
                    }
                }
<?              break;
            case 'single': ?>
                gotResult = true;
                gla.GetResult(<?=args($innerOutputs)?>);
                ++it;
<?              break;
            case 'state':
                reset($innerOutputs);
                // Assuming that $innerOutputs contains a single value that is
                // the state type.
                $oName = key($innerOutputs);
                $oType = current($innerOutputs);
?>
                gotResult = true;
                <?=$oName?> = <?=$oType?>( &gla );
                ++it;
<?      } // switch inner result type ?>
            }

            return gotResult;
        }
    };

private:
    const ConstantState & constState;

<?  if( $configurable ) { ?>
    const Json::Value jsonInit;
<?  } // if configurable ?>

    size_t count;

    MapType groupByMap;

    std::vector<MapType::iterator> theIterators;  // the iterators, only 2 elements if multi, many if fragment
    Iterator multiIterator;

public:

    <?=$className?>(<? if($configurable) {?>const Json::Value & _jsonInit, <? } ?>const ConstantState & _constState ) :
        constState(_constState)
<?  if( $configurable ) { ?>
        , jsonInit(_jsonInit)
<?  } // if configurable ?>
        , count(0)
        , groupByMap( INIT_SIZE )
        , theIterators()
        , multiIterator()
    { }

    ~<?=$className?>() {}

    void Reset(void) {
        count = 0;
        groupByMap.clear();
        theIterators.clear();
    }

    void AddItem(<?=array_template('const {val} & {key}', ', ', $inputs)?>) {
        count++;
        // check if _key is already in the map; if yes, add _value; else, add a new
        // entry (_key, _value)
        Key key(<?=array_template('{key}', ', ', $gbyAtts)?>);

        MapType::iterator it = groupByMap.find(key);
        if (it == groupByMap.end()) { // group does not exist
            // create an empty GLA and insert
            // better to not add the item here so we do not have
            // to transport a large state

<?  if( $innerGLA->has_state() ) { ?>
            const InnerState & innerState = constState.getConstState(key);
<?  } // if gla has state ?>
            InnerGLA gla<?=$constructorString?>;
            auto ret = groupByMap.insert(MapType::value_type(key, gla));
            it = ret.first; // reposition
        }
        it->second.AddItem(<?=array_template('{key}', ', ', $glaInputAtts)?>);
    }

    void AddState(<?=$className?>& other) {
        count += other.count;
        // scan other hash and insert or update content in this one
        for (MapType::iterator it = other.groupByMap.begin(); it != other.groupByMap.end();
                ++it) {
            const Key& okey = it->first;
            <?=$innerGLA?>& ogla = it->second;

            MapType::iterator itt = groupByMap.find(okey);
            if (itt != groupByMap.end()) { // found the group
                <?=$innerGLA?>& gla = itt->second;
                gla.AddState(ogla);
            } else {
                // add the other group to this hash
                groupByMap.insert(MapType::value_type(okey, ogla));
            }
        }
    }

<?  if( $iterable ) { ?>
    bool ShouldIterate(ConstantState& modibleState) {
<?      if( $debug > 0 ) { ?>
        fprintf(stderr, "<?=$className?>: ==== ShouldIterate ====\n");
<?      } // if debugging enabled ?>
        bool shouldIterate = false;
        for( MapType::iterator it = groupByMap.begin(); it != groupByMap.end(); ++it ) {
            const Key & key = it->first;
            InnerGLA & gla = it->second;
<?  if( $innerGLA->has_state() ) { ?>
            InnerState & innerState = modibleState.getModibleState(key);
<?  } // if gla has state ?>
            bool glaRet = gla.ShouldIterate(innerState);
            shouldIterate = shouldIterate || glaRet;
<?      if( $debug > 0 ) { ?>
            fprintf(stderr, "<?=$className?>: Key(%s) shouldIterate(%s)\n",
                key.to_string().c_str(),
                glaRet ? "true" : "false");
<?      } // if debugging enabled ?>
        }

        return shouldIterate;
    }
<?  } // if iterable ?>

<?
    if( in_array( 'fragment' , $resType) ) {
?>

    int GetNumFragments(void){
        int size = groupByMap.size();
        int sizeFrag = <?=$fragSize?>;
        // setup the fragment boundaries
        // scan via iterator and count
        int frag=0;
        int pos=0;
        MapType::iterator it = groupByMap.begin();
        theIterators.clear();
        theIterators.push_back( it );
        // special case when size < num_fragments
        // >
        if (sizeFrag == 0){
            it = groupByMap.end();
            theIterators.push_back( it );
            return 1; // one fragment
        }

        while(it!=groupByMap.end()){
            while(it!=groupByMap.end() && pos<( frag + 1 )*sizeFrag){
//>
                ++it;
                pos++;
            }
            theIterators.push_back( it );
            frag++;
        }

<?php if($debug > 0) { ?>
        fprintf(stderr, "<?=$className?>: fragments(%d)\n", frag);
<?php } ?>

        return frag;

    }

    Iterator* Finalize(int fragment){
        // Call finalize on all inner GLAs in this fragment.
        MapType::iterator iter = theIterators[fragment];
        MapType::iterator iterEnd = theIterators[fragment+1];

        Iterator* rez
            = new Iterator(theIterators[fragment], theIterators[fragment+1] );
        return rez;
    }

    bool GetNextResult(Iterator* it,  <?=array_template('{val} & {key}', ', ', $outputs)?>) {
        return it->GetNextResult(<?=args($outputs)?>);
    }
<?
    } // if using fragment interface
?>

    void Finalize() {
        multiIterator = Iterator( groupByMap.begin(), groupByMap.end() );

<?  if( $debug >= 1 ) { ?>
        fprintf(stderr, "<?=$className?>: groups(%lu) tuples(%lu)\n", groupByMap.size(), count);
<?  } ?>
    }

    bool GetNextResult(<?=array_template('{val} & {key}', ', ', $outputs)?>) {
        return multiIterator.GetNextResult( <?=args($outputs)?> );
    }

    std::size_t size() const {
        return groupByMap.size();
    }

    const MapType& GetMap() const {
      return groupByMap;
    }

    bool Contains(<?=const_typed_ref_args($gbyAtts)?>) const {
      Key key(<?=args($gbyAtts)?>);
      return groupByMap.count(key) > 0;
    }

    const InnerGLA& Get(<?=const_typed_ref_args($gbyAtts)?>) const {
      Key key(<?=args($gbyAtts)?>);
      return groupByMap.at(key);
    }

    bool Contains(Key key) const {
      return groupByMap.count(key) > 0;
    }

    const InnerGLA& Get(Key key) const {
      return groupByMap.at(key);
    }
};

<?  if( in_array( 'fragment', $resType ) ) { ?>
typedef <?=$className?>::Iterator <?=$className?>_Iterator;
<?  } ?>

<?
    $sys_headers = array_merge(['iomanip', 'iostream', 'cstring'], $extraHeaders);

    return array(
        'kind'             => 'GLA',
        'name'             => $className,
        'system_headers'   => $sys_headers,
        'user_headers'     => array('HashFunctions.h'),
        'input'            => $inputs,
        'output'           => $outputs,
        'result_type'      => $resType,
        'configurable'     => $configurable,
        'generated_state'  => $constState,
        'required_states'  => $reqStates,
        'iterable'         => $iterable,
        'properties'       => [ 'resettable', 'finite container' ],
        'libraries'        => $libraries,
        'extra'            => [ 'inner_gla' => $innerGLA],
    );
}
?>
