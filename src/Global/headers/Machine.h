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
#ifndef _MACHINE_H_
#define _MACHINE_H_

/** This header file contains machine specific macros. It shuld be
    included by all programs that depend on specifics of a machine */

// the size of a word in bytes. Used to ensure alignment
#define WORDSIZE sizeof(int)

// macro to convert number of bytes to words
#define BYTES_TO_WORDS(x)			\
	( (x) % WORDSIZE == 0? (x)/WORDSIZE : (x)/WORDSIZE+1 )

// macro to transform number of bytes to number of bytes that are a multiple of
#define BYTES_TO_WORD_ALIGNED(x)		\
	(BYTES_TO_WORDS(x)*WORDSIZE)

#endif //_MACHINE_H_
