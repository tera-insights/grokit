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
#include <cinttypes>
#include <limits>

/*
 *  This header defines functions used for thread-safe random number generation
 *  that will not lock when different threads attempt to generate random
 *  numbers.
 */

/**
 * Generates a random integer on the closed interval [start, end]
 * @param  start The lower bound on numbers to be generated
 * @param  end   The upper bound on numbers to be generated
 * @return       A random integer in the range [start, end]
 */
int64_t RandInt(int64_t start = 0, int64_t end = std::numeric_limits<uint64_t>::max());

double RandDouble(void);

#endif // _DP_RANDOM_H_
