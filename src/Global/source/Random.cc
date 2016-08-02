//
//  Copyright 2012 Christopher Dudley
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//

#include "Random.h"
#include "Errors.h"

#ifdef _HAS_STD_RANDOM

#include <random>

// Use STL pseudo-random number generators with thread-local storage.

namespace {
    struct RandomState {
        using random_t = std::mt19937_64;


        bool initialized;
        random_t rng;

        RandomState(): initialized(false), rng() { }

        void init() {
            std::random_device system_rand;

            // 64 bits of seed material
            uint32_t seed_vec[2];
            seed_vec[0] = system_rand();
            seed_vec[1] = system_rand();

            std::seed_seq seed(seed_vec, seed_vec + 2);

            rng.seed(seed);

            initialized = true;
        }
    };

    THREAD_LOCAL RandomState rand_state;
}

int64_t RandInt(int64_t start, int64_t end) {
    if (!rand_state.initialized) {
        rand_state.init();
    }

    std::uniform_int_distribution<int64_t> dist(start, end);
    return dist(rand_state.rng);
}

double RandDouble(void)  {
    if (!rand_state.initialized) {
        rand_state.init();
    }

    return std::generate_canonical<double, 32>(rand_state.rng);
}

std::size_t RandBytes(uint8_t* start, uint8_t* end) {
    using data_t = RandomState::random_t::result_type;

    if (!rand_state.initialized) {
        rand_state.init();
    }

    data_t data;
    std::size_t bitsLeft = 0;
    std::size_t bytesGen = 0;
    while (start != end) {
        if (bitsLeft < 8) {
            data = rand_state.rng();
            bitsLeft = RandomState::random_t::word_size;
        }

        *start = uint8_t(data & 0xFF);
        data >>= 8;
        bitsLeft -= 8;
        bytesGen++;
        start++;
     }

     return bytesGen;
}

#else // _HAS_STD_RANDOM

#include <cstdlib>
#include <cstdio>
#include <limits>

// Use rand48 class of functions with thread-local storage for the state.

namespace {
    struct RandomState {
        // Same thing as above. Don't seed the RNG unless we need to use
        // it in a thread.

        bool initialized;
        drand48_data data;

        RandomState(): initialized(false) { }

        void init() {
            FILE* rd = fopen("/dev/urandom", "r");
            unsigned short seed[3];
            fread(seed, sizeof(unsigned short int), 3, rd);
            fclose(rd);

            seed48_r(seed, &data);

            initialized = true;
        }
    };

    THREAD_LOCAL RandomState rand_state;
}

int64_t RandInt(int64_t start, int64_t end) {
    FATALIF(start > end, "RandInt: start > end");

    if (!rand_state.initialized) {
        rand_state.init();
    }

    uint64_t range = (end - start) + 1;
    int64_t result;
    long tmp;

    // mrand48 returns integers between -2^31 and 2^31, so
    // combine two of them to create a 64-bit random number
    mrand48_r(&rand_state.data, &tmp);
    result = tmp & 0xFFFFFFFFLL; // Only keep the lower 32 bits
    mrand48_r(&rand_state.data, &tmp);
    result |= tmp << 32;             // Add this number to the upper 32 bits

    if (range == 0) {
        // start is minimum int64_t and end is maximum int64_t
        // just return the result, it's already randomly distributed
        // in that range.
        return result;
    } else {
        return start + (result % range);
    }
}

double RandDouble(void) {
    if (!rand_state.initialized) {
        rand_state.init();
    }

    double result;
    drand48_r(&rand_state.data, &result);
}

std::size_t RandBytes(uint8_t* start, uint8_t* end) {
    if (!rand_state.initialized) {
        rand_state.init();
    }

    uint32_t data;
    long tmp;
    std::size_t bitsLeft;
    std::size_t bytesGen;
    while (start != end) {
        if (bitsLeft < 8) {
            mrand48_r(&rand_state.data, &tmp);
            data = uint32_t(tmp & 0xFFFFFFFFLL);
            bitsLeft = 32;
        }

        *start = uint8_t(data & 0xFF);
        data >>= 8;
        bitsLeft -= 8;
        bytesGen++;
        start++;
     }

     return bytesGen;
}

#endif // _HAS_STD_RANDOM
