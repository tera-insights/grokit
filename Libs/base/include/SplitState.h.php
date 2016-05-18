<?

// Copyright 2014 Tera Insights, LLC. All Rights Reserved.

function SplitState(array $t_args) {
    $sys_headers = [ 'mutex', 'condition_variable', 'array', 'random' ];
    $user_headers = [];

    grokit_assert(array_key_exists('type', $t_args), 'SplitState: No type given');
    grokit_assert(array_key_exists('size', $t_args), 'SplitState: No size given');

    $type = $t_args['type'];
    $size = $t_args['size'];

    $className = generate_name('SplitState_');
?>
class <?=$className?> {
public:
    using StateType = <?=$type?>;

    static constexpr size_t NUM_STATES = <?=$size?>;
    using UniqueLock =  std::unique_lock<std::mutex>;

    using StateArray = std::array<StateType *, NUM_STATES>;
    using BoolArray = std::array<bool, NUM_STATES>;

private:
    // Array of states
    StateArray stateArray;

    // Mutex to protect states
    std::mutex myMutex;

    // Condition variable to wake up threads blocked on acquiring a state.
    std::condition_variable signalVar;

    // Keeps track of which states are available to be checked out.
    BoolArray writeLocked;

    // Random number generator
    std::mt19937_64 rng;

public:

    // Constructor
    <?=$className?>( ) :
        stateArray(),
        myMutex(),
        signalVar(),
        writeLocked(),
        rng()
    {
        stateArray.fill(nullptr);
        writeLocked.fill(false);

        std::random_device rd;

        // 64-bits of seed
        uint32_t seed_vals[2];
        seed_vals[0] = rd();
        seed_vals[1] = rd();

        std::seed_seq seed(seed_vals, seed_vals + 2);

        rng.seed(seed);
    }

    // Destructor
    ~<?=$className?>() {
        for( auto elem : stateArray ) {
            if( elem != nullptr ) {
                delete elem;
            }
        }
    }

    // Methods

    int CheckOutOne( int *theseAreOK, StateType *& checkMeOut ) {
        // first, figure out all of the OK segments
        int numWanted = 0;
        int goodOnes[NUM_STATES];
        for (int i = 0; i < NUM_STATES; i++) { //>
            if (theseAreOK[i] == 1) {
                goodOnes[numWanted] = i;
                numWanted++;
            }
        }

        {   UniqueLock lock(myMutex); // Acquire lock

            // now, try them one-at-a-time, in random order
            while (1) {

                // try each of the desired hash table segments, in random order
                for (int i = 0; i < numWanted; i++) { //>

                    // randomly pick one of the guys in the list
                    std::uniform_int_distribution<int> dist(i, numWanted-1);
                    int whichIndex = dist(rng);

                    // move him into the current slot
                    int whichToChoose = goodOnes[whichIndex];
                    goodOnes[whichIndex] = goodOnes[i];
                    goodOnes[i] = whichToChoose;

                    // try him
                    if (!writeLocked[whichToChoose]) {

                        // he is open, so write lock him
                        writeLocked[whichToChoose] = true;

                        // and return him
                        checkMeOut = stateArray[whichToChoose];
                        stateArray[whichToChoose] = nullptr;
                        return whichToChoose;
                    }
                }

                // if we got here, then every one that we want is write locked.  So
                // we will go to sleep until one of them is unlocked, at which point
                // we will wake up and try again...
                signalVar.wait(lock);
            }
        }
    }

    void CheckIn( int whichEntry, StateType *& checkMeIn ) {
        // just note that no one is writing this one, then signal all potential writers
        {
            UniqueLock lock(myMutex);
            writeLocked[whichEntry] = false;

            stateArray[whichEntry] = checkMeIn;
            checkMeIn = nullptr;
        }

        signalVar.notify_all();
    }

    StateType * Peek( int whichEntry ) {
        return stateArray[ whichEntry ];
    }

    void Delete( int whichEntry ) {
        if( stateArray[whichEntry] != nullptr ) {

            delete stateArray[whichEntry];
            stateArray[whichEntry] = nullptr;
        }
    }

    void Reset() {
        for(size_t i = 0; i < NUM_STATES; i++) {
<?  if( $type->is('resettable') ): ?>
            if( stateArray[i] != nullptr ) {
                stateArray[i]->Reset();        
            }
<?  else: ?>
            Delete(i);
<?  endif; // not resettabile ?>
        }
    }
};
<?
    return [
        'kind'              => 'RESOURCE',
        'name'              => $className,
        'system_headers'    => $sys_headers,
        'user_headers'      => $user_headers,
    ];
}

?>
