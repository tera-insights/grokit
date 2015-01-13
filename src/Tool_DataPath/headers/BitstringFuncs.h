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
#ifndef BIT_H_
#define BIT_H_
#include "Hash.h"

int GetSerializedSize (Bitstring64 a) {
	return sizeof (Bitstring64);
}

// note: if this function does not put the result in here, it
// MUST reutrn a pointer to at least sizeof (VAL_TYPE) valid bytes!!!
void *OptimizedBinarySerialize (const Bitstring64 a, void *here) {
	*((Bitstring64 *) here) = a;
}

#endif
