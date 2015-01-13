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
#ifndef _DATA_TYPE_DEF_MACRO_H_
#define _DATA_TYPE_DEF_MACRO_H_

/** This header contains macros that make data type definitions much
		more pleasant. More specifically, for all fixed data types, the
		required functions, except To/From string are automatically
		defined. For variable sized datatypes, functions that call member
		functions, so the first argument can be omitted, are defined.

		The macros defined here have to be used in the file describing the
		data type AFTER the definition of the type.
*/


/** This macro delares a datatype as being basic and fixed
		size. There is no way to check the correctness of this in
		C++ so please use with care. What this macro indicates is that the
		object's binary represenation is captured by the sizeof(DataType)
		bits.

		We still expect the user to provide functions To/From String

*/

#define DECLARE_SIMPLE_DATATYPE(DataType)				\
	


#endif // _DATA_TYPE_DEF_MACRO_H_
