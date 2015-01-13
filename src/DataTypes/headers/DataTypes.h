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
#ifndef _DATATYPES_H_
#define _DATATYPES_H_

#include <inttypes.h>

/** This header file contains further includes to ensure that all the
    basic datatypes supported are available. It also captures the
    information in an array to make it easy for the AttributeType
    class to manipulate them. The reson for not defining all this info
    in AttributeType is to keep the number of files to be updated when
    a new type is introduced to a minimum.

    NOTE: when a new datatype is added, this file needs
    to be updated. The following 3 updates are needed:

		1. The file containing the new type in datatypes directory should
		be included

		2. The new type should be defined in AttributeTypesEnum

		3. The info about the type and all the sypported synonnims are
		added to the array dataTypeInfo. The variable dataTypeInfoSize
		should be update as well.

*/

#if 0
// all int types
#include "INT.h"

// all big int types
#include "BIGINT.h"

// the single precision floads
#include "FLOAT.h"

// the doule precision floats
#include "DOUBLE.h"

//the date datatype
#include "DATE.h"
//#include "date.h"

// the ipv4 addresses
#include "IPV4ADDR.h"

// the MAC addresses
#include "MACADDR.h"

#include "STRING_LITERAL.h"

// STATE datatype
#include "STATE.h"
#endif

#include <string.h>

/** The category should almost never be added to since they are much
		harder to add to the system. A special category, CExactMatch is
		defined that specifies that the type only matches itself.
*/

enum TypeCategoryEnum {
	CExactMatch, /* The types in this category are only compatible with themselves */
	CNumeric, /* all the types for which operations with other numbers are well defined */
	CDate, /* all date types, i.e. they have the same interface as the date type */
	CString, /* all string types, i.e. they support string operations */
};


/** The following enum gives names to all the types supported by the
		AttributeType class. This is defined here to avoid splitting the
		info into too many files.
*/

enum AttributeTypeEnum {
	TUnknown, /* Type used for de default value */
	TInteger,
	TBigInt,
	TFloat,
	TDouble,
	TDate,
	TCharN,
	TVarChar,
	TDString,
	TIPaddr
};

struct DataTypeEl{
	const char* name; // name of the type without size arguments
	AttributeTypeEnum tType; // the type
	TypeCategoryEnum tCat; // the category it belongs to
	bool hasSize; // if true, a size parameter should be suplied
};

// global variable with all the types in the system
/* default behavior, we just declare */
extern DataTypeEl dataTypeInfo[16];

#endif // _DATATYPES_H_
