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
#ifndef _INTFUNCS_H_
#define _INTFUNCS_H_
#include "Hash.h"

HT_INDEX_TYPE Hash (int a) {
	return CongruentHash (a);
}

int GetSerializedSize (int a) {
	return sizeof (int);
}

// note: if this function does not put the result in here, it
// MUST reutrn a pointer to at least sizeof (VAL_TYPE) valid bytes!!!
void *OptimizedBinarySerialize (int a, void *here) {
	*((int *) here) = a;
	return here;
}


#endif
