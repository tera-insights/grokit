//
//  Copyright 2012 Alin Dobra and Christopher Jermaine
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
#ifndef _BITSTRING_H_
#define _BITSTRING_H_

#include <iostream>
#include "Bitstring.h"
#include "Constants.h"
#include "Machine.h"
#include <assert.h>
#include <cstring>
#include <cstdio>

/** These are wrapper classes to read and write data in Column using BStringIterator, we have so far 3 types
  depending on use modes. Which mode to use depsnds on pattern length we need, for small patterns where
  only few queries are being used (less than equalto 16), we can use 16 bit pattern class BString16_16.
  BString32_64 supports full 64 bit bitstring pattern.
  */

// This one maintains 16 bit pattern and 16 bit counter (number of times)
class BString16_16 {
    public:
        unsigned short mPattern;
        unsigned short mRepeatCount;

        BString16_16(unsigned short _pattern, unsigned short _count) :
            mPattern(_pattern), mRepeatCount(_count) {}

        operator Bitstring () {
            return Bitstring(mPattern, true);
        }

        unsigned int GetObjLength() const {
            return 2 * sizeof (unsigned short);
        }

        unsigned short GetCount() const {
            return mRepeatCount;
        }

        unsigned short GetPattern() const {
            return mPattern;
        }
};

// This one maintains 32 bit pattern and 32 bit counter (number of times)
class BString32_32 {
    public:
        unsigned int mPattern;
        unsigned int mRepeatCount;

        BString32_32(unsigned int _pattern, unsigned int _count) :
            mPattern(_pattern), mRepeatCount(_count) {}

        operator Bitstring () {
            return Bitstring(mPattern, true);
        }

        unsigned int GetObjLength() const {
            return 2 * sizeof (unsigned int);
        }

        unsigned int GetCount() const {
            return mRepeatCount;
        }

        unsigned int GetPattern() const {
            return mPattern;
        }
};

// This one maintains 64 bit pattern and 32 bit counter (number of times)
class BString32_64 {
    public:
        __uint64_t mPattern;
        unsigned int mRepeatCount;

        BString32_64(__uint64_t _pattern, unsigned int _count) :
            mPattern(_pattern), mRepeatCount(_count) {}

        operator Bitstring () {
            return Bitstring(mPattern, true);
        }

        unsigned int GetObjLength() const {
            return sizeof(__uint64_t) + sizeof (unsigned int);
        }

        unsigned int GetCount() const {
            return mRepeatCount;
        }

        __uint64_t GetPattern() const {
            return mPattern;
        }
};

#endif
