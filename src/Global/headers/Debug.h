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
#ifndef _DEBUG_H
#define _DEBUG_H

#include <stdio.h>

/** This header file has a set of handy macros for assertions and
	* debugging.
	*
	* Please use this extensively. Keep in mind this is for debugging
	* only. If you want to report errors outside of debugging mode,
	* take a look at Errors.h.
	*/


/** Prints a message if debugging mode is enabled */
#ifdef DEBUG
#define PDEBUG(msg...) { 					\
		printf("DEBUG [%s:%d]: ", __FILE__, __LINE__);		\
		printf(msg);						\
		printf("\n");						\
	}
#else
#define PDEBUG(msg...) ;
#endif

#ifdef DEBUG
#define PDEBUGIF(expr,msg...) {				\
		if (expr) {						\
			printf("DEBUG [%s:%d]: ", __FILE__, __LINE__);	\
			printf(msg);					\
			printf("\n");					\
		}							\
	}
#else
#define PDEBUGIF(expr,msg...) ;
#endif

#endif // _DEBUG_H
