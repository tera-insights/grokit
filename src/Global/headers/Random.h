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
#ifndef _DP_RANDOM_H_
#define _DP_RANDOM_H_

#include "Config.h"

/*
 *  This header defines functions used for thread-safe random number generation
 *  that will not lock when different threads attempt to generate random
 *  numbers.
 */

#ifdef _HAS_STD_RANDOM

// Use STL pseudo-random number generators with thread-local storage.

#include <random>

unsigned long RandInt(void);

double RandDouble(void);

#else // _HAS_STD_RANDOM

// Use rand48 class of functions with thread-local storage for the state.
#include <cstdlib>
#include <cstdio>

unsigned long int RandInt(void); 

double RandDouble(void); 

#endif // _HAS_STD_RANDOM

#endif // _DP_RANDOM_H_
