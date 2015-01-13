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

#ifdef _HAS_STD_RANDOM

// Use STL pseudo-random number generators with thread-local storage.

THREAD_LOCAL std::mt19937* _dp_rand_gen = NULL;

unsigned long RandInt(void) {
    if( __builtin_expect( _dp_rand_gen == NULL, false ) ) {
        std::random_device rd;
        _dp_rand_gen = new std::mt19937(rd());
    }

    return (*_dp_rand_gen)();
}

double RandDouble(void)  {
    if( __builtin_expect( _dp_rand_gen == NULL, false ) ) {
        std::random_device rd;
        _dp_rand_gen = new std::mt19937(rd());
    }

    return std::generate_canonical<double, 32>(*_dp_rand_gen);
}

#else // _HAS_STD_RANDOM

// Use rand48 class of functions with thread-local storage for the state.

THREAD_LOCAL unsigned short int* _dp_rand_data = NULL;

unsigned long int RandInt(void) {
    if( __builtin_expect( _dp_rand_data == NULL, false ) ) {
        _dp_rand_data = new unsigned short int[3];

        FILE* rd = fopen("/dev/urandom", "r");
        fread( _dp_rand_data, sizeof(unsigned short int), 3, rd );
        fclose( rd );
    }

    return nrand48( _dp_rand_data );
}

double RandDouble(void) {
    if( __builtin_expect( _dp_rand_data == NULL, false ) ) {
        _dp_rand_data = new unsigned short int[3];

        FILE* rd = fopen("/dev/urandom", "r");
        fread( _dp_rand_data, sizeof(unsigned short int), 3, rd );
        fclose( rd );
    }

    return erand48( _dp_rand_data );
}

#endif // _HAS_STD_RANDOM
