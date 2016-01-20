<?
// Copyright 2014 Tera Insights, LLC. All Rights Reserved.

function SegmenterState(array $targs) {
    $classname = generate_name('SegmenterState');

     $gla = $targs['gla'];
     $npasses = $targs['passes'];
     $segments = $targs['segments'];

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

     $sys_headers = ['cstddef'];

    $splitState = lookupResource('BASE::SplitState', [
        'type' => $gla,
        'size' => $segments
    ]);

     $iterable = $npasses > 1;
?>

struct <?=$classname?> {
    using SplitState = <?=$splitState?>;

    static constexpr const size_t N_PASSES = <?=$npasses?>;

    mutable SplitState segments;

    // Current pass number
    size_t pass;

<?  if($gla->has_state()): ?>
    // Inner GLAs constant state
    <?=$gla->state()?> inner_cstate;
<?  endif; // gla has state ?>

    <?=$classname?>(<?=const_typed_ref_args($cArgs)?>):
        segments()
        , pass(0)
<?  if($gla->has_state()): ?>
        , inner_cstate(<?=args($cArgs)?>)
<?  endif; // gla has state ?>
    {}

    void Reset() {

    }
};

<?
    return [
        'kind'              => 'RESOURCE',
        'name'              => $classname,
        'system_headers'    => $sys_headers,
        'configurable'      => $configurable,
    ];
}

function Segmenter( array $t_args, array $input, array $output, array $given_states) {
    $resType = [ 'fragment', 'multi' ];
    $system_headers = [ 'array', 'vector', 'memory', 'cinttypes', 'unordered_map' ];
    $user_headers = [ 'HashFunctions.h' ];
    $lib_headers = [];

    $preferFragment = get_default($t_args, 'inner.prefer.fragment', false);
    $wantedRes = $preferFragment ? ['fragment', 'multi'] : ['multi', 'fragment'];

    $nInputs = \count($input);

    grokit_assert($nInputs > 1, 'Segmenter: Not enough inputs specified!');

    $keyName = array_keys($input)[0];
    $keyType = array_get_index($input, 0);

    $innerInputs = array_slice($input, 1, $nInputs - 1, true);

    $gla = get_first_key($t_args, [ 'gla', 'GLA', 0 ]);
    grokit_assert( is_gla($gla), 'Segmenter: [gla] argument must be a valid GLA');
    $gla = $gla->apply($innerInputs, $output, $given_states);

    $n_passes = get_default($t_args, 'passes', 1);
    grokit_assert(is_int($n_passes), 'Segmenter: [passes] argument must be an integer');
    grokit_assert($n_passes > 0, 'Segmenter: [passes] argument must be > 0');

    $libraries = $gla->libraries();

    $innerRes = get_first_value($gla->result_type(), $wantedRes);
    $innerInputs = $gla->input();
    $innerOutput = $gla->output();
    $input = array_merge([ $keyName => $keyType ], $innerInputs);
    $output = $innerOutput;

    $segments = get_default($t_args, 'segments', 64);
    $constState = lookupResource('BASE::SegmenterState', [
        'gla' => $gla,
        'passes' => $n_passes,
        'segments' => $segments
    ]);

    $className = generate_name('Segmenter_');

    $savedArgs = [];
    $cArgs = [];
    $innerCArgs = [];
    if( $gla->configurable() ) {
        $savedArgs['json_init'] = 'Json::Value';
        $cArgs['json_init'] = 'Json::Value';
        $innerCArgs[] = 'json_init';
    }

    $cArgs['const_state'] = $constState;
    if( $gla->has_state() ) {
        $innerCArgs[] = 'constState.inner_cstate';
    }

    $cstStr = \count($innerCArgs) > 0 ? '(' . implode(',',$innerCArgs) . ')' : '';

    grokit_assert(!$gla->iterable(), 'Segementer does not support iterable GLAs');

    $iterable = $n_passes > 1;
?>

class <?=$className?> {
private:

    using ConstantState = <?=$constState?>;
    using SplitState = ConstantState::SplitState;

    static constexpr const size_t NUM_STATES = SplitState::NUM_STATES;

    using InnerGLA = <?=$gla?>;
    using InnerGLAPtr = std::unique_ptr<InnerGLA>;
    using GLA_Array = std::array<InnerGLAPtr, NUM_STATES>;

public:
    using size_type = std::size_t;

<?  if( $innerRes == 'fragment' ) { ?>
    class Iterator {
    private:
        InnerGLA * gla;
        int fragmentNum;

        InnerGLA::Iterator * innerIter;

    public:
        Iterator( InnerGLA * _gla, int _fragmentNum, int _innerFrag ) :
            gla(_gla), fragmentNum(_fragmentNum),
            innerIter(nullptr)
        {
            innerIter = gla->Finalize(_innerFrag);
        }

        ~Iterator(void) {
            if( innerIter != nullptr ) {
                delete innerIter;
                innerIter = nullptr;
            }
        }

        bool GetNextResult( <?=typed_ref_args($gla->output())?> ) {
            return innerIter->GetNextResult(<?=args($gla->output())?>);
        }

        int FragmentNumber() {
            return fragmentNum;
        }
    };
<?  } else { // if inner result type is fragment ?>
    class Iterator {
    private:
        InnerGLA * gla;
        int fragmentNum;

    public:
        Iterator( InnerGLA * _gla, int fragNo ) : gla(_gla), fragmentNum(fragNo) {
            gla->Finalize();
        }

        ~Iterator(void) { }

        bool GetNextResult( <?=typed_ref_args($gla->output())?> ) {
            return gla->GetNextResult(<?=args($gla->output())?>);
        }

        int FragmentNumber() {
            return fragmentNum;
        }
    };
<?  } // if inner result type is multi ?>

private:

    const ConstantState & constState;
    GLA_Array localState;

    // Iteration state for multi result type
    int numFrags;
    int multiFragNo;
    Iterator * multiIter;

<?php if($innerRes == 'fragment') { ?>
    using frag_info = std::pair<int, int>;
    using frag_map_t = std::unordered_map<int, frag_info>;
    frag_map_t fragMap;
<?php } ?>


<?  foreach($savedArgs as $name => $type) { ?>
    const <?=$type?> <?=$name?>;
<?  } // foreach saved arg ?>

public:

    // Constructor
    <?=$className?>( <?=const_typed_ref_args($cArgs)?> ) :
        constState(const_state)
        , localState()
        , numFrags(0)
        , multiFragNo(0)
        , multiIter(nullptr)
<?  if ($innerRes == 'fragment') { ?>
        , fragMap()
<?  } ?>
<?  foreach($savedArgs as $name => $type) { ?>
        , <?=$name?>(<?=$name?>)
<?  } // foreach constructor arg to save ?>
    {
        for( auto & elem : localState ) {
            elem.reset(new InnerGLA<?=$cstStr?>);
        }
    }

    void AddItem( <?=const_typed_ref_args($input)?> ) {
        uint64_t hashVal = CongruentHash(Hash(<?=$keyName?>), H_b + 1);
        uint64_t passNum = (hashVal / NUM_STATES) % ConstantState::N_PASSES;
        uint64_t segNum = hashVal % NUM_STATES;

<?  if( $n_passes > 1 ): ?>
        if( passNum != constState.pass ) {
            return;
        }
<?  endif; // more than 1 pass ?>
        localState[segNum]->AddItem(<?=args($innerInputs)?>);
    }

    void ChunkBoundary(void) {
        // Merge local states into the global state

        SplitState & globalStates = constState.segments;

        int theseAreOk[NUM_STATES];
        for( int i = 0; NUM_STATES > i; i++ ) {
            theseAreOk[i] = 1;
        }

        int segsLeft = NUM_STATES;

        while( segsLeft > 0 ) {
            InnerGLA * checkedOut = nullptr;
            int whichOne = globalStates.CheckOutOne( theseAreOk, checkedOut );

            if( checkedOut == NULL ) {
                checkedOut = new InnerGLA<?=$cstStr?>;
            }

            checkedOut->AddState( *(localState[whichOne]) );

            globalStates.CheckIn( whichOne, checkedOut );

            theseAreOk[whichOne] = 0;
            segsLeft--;
        }

        // Re-initialize the local states
        for( auto & elem : localState ) {
<?  if( $gla->is('resettable') ) { ?>
            elem->Reset();
<?  } else { // if resettable ?>
            elem.reset(new InnerGLA<?=$cstStr?>);
<?  } // if not resettable ?>
        }
    }

    void AddState( <?=$className?> & o ) {
        // Do nothing
    }

    void Finalize() {
        SplitState & globalStates = constState.segments;

        if( multiIter != nullptr)
            delete multiIter;

        multiFragNo = 0;
<?  if ($innerRes == 'fragment') {?>
        frag_info fInfo = fragMap[multiFragNo];
        multiIter = new Iterator(globalStates.Peek(fInfo.first), multiFragNo,
            fInfo.second);
<?  } else { ?>
        multiIter = new Iterator(globalStates.Peek(multiFragNo), multiFragNo);
<?  } ?>
    }

    bool GetNextResult(<?=typed_ref_args($output)?>) {
        bool gotResult = false;
        SplitState & globalStates = constState.segments;

        while( (multiFragNo < numFrags && multiIter != nullptr) && !gotResult ) {
            gotResult = multiIter->GetNextResult(<?=args($output)?>);

            if( !gotResult ) {
                multiFragNo++;
                delete multiIter;

                if( numFrags > multiFragNo ) {
<?  if ($innerRes == 'fragment') {?>
                    frag_info fInfo = fragMap[multiFragNo];
                    multiIter = new Iterator(globalStates.Peek(fInfo.first), multiFragNo,
                        fInfo.second);
<?  } else { ?>
                    multiIter = new Iterator(globalStates.Peek(multiFragNo), multiFragNo);
<?  } ?>
                } else {
                    multiIter = nullptr;
                }
            }
        }

        return gotResult;
    }

    int GetNumFragments(void) {
<?  if ($innerRes == 'fragment') {?>
        SplitState & globalStates = constState.segments;
        numFrags = 0;

        for (int i = 0; i < NUM_STATES; i++) {
            int curFrags = globalStates.Peek(i)->GetNumFragments();

            for (int curFrag = 0; curFrag < curFrags; curFrag++) {
                fragMap[numFrags] = frag_info(i, curFrag);

                numFrags++;
            }
        }
<?  } else { ?>
        numFrags = NUM_STATES;
<?  } ?>
        return numFrags;
    }

    Iterator * Finalize( int fragment ) {
        SplitState & globalStates = constState.segments;

<?  if ($innerRes == 'fragment') { ?>
        frag_info info = fragMap[fragment];
        return new Iterator(globalStates.Peek(info.first), fragment, info.second);
<?  } else { ?>
        return new Iterator(globalStates.Peek(fragment), fragment);
<?  } ?>
    }

    bool GetNextResult( Iterator * it, <?=typed_ref_args($output)?> ) {
        bool ret = it->GetNextResult(<?=args($output)?>);

        return ret;
    }

<?  if($iterable): ?>
    bool ShouldIterate( ConstantState & modible ) {
        modible.pass++;

        return modible.pass < ConstantState::N_PASSES;
    }

    void PostFinalize() {
        constState.segments.Reset();
    }
<?  endif; // iterable ?>

<?  if( $gla->is('finite container') ) { ?>
    size_type size() {
        SplitState & globalStates = constState.segments;
        size_type s = 0;
        for( int i = 0; NUM_STATES > i; i++ ) {
            InnerGLA * ptr = globalStates.Peek(i);
            s += ptr->size();
        }

        return s;
    }

    size_type size(int frag) {
        SplitState & globalStates = constState.segments;
        return globalStates.Peek(frag)->size();
    }
<?  } // if the gla is a container ?>
};

typedef <?=$className?>::Iterator <?=$className?>_Iterator;

<?
    return [
        'kind'              => 'GLA',
        'name'              => $className,
        'system_headers'    => $system_headers,
        'user_headers'      => $user_headers,
        'lib_headers'       => $lib_headers,
        'libraries'         => $libraries,
        'input'             => $input,
        'output'            => $output,
        'result_type'       => $resType,
        'generated_state'   => $constState,
        'required_states'   => $gla->req_states(),
        'chunk_boundary'    => true,
        'configurable'      => $gla->configurable(),
        'iterable'          => $iterable,
        'post_finalize'     => $iterable,
        'intermediates'     => true,
    ];
}
?>
