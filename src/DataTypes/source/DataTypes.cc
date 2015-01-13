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
#include "DataTypes.h"


/** NOTE: the array dataTypeInfo is always scanned in order. It is
		important, thus, to put the prefered name first among synonnims.
*/

/* DO NOT FORGET TO UPDATE THIS WHEN ADDING ITEMS */
DataTypeEl dataTypeInfo[16]={
	/** Default */
	{"UNKNOWN", TUnknown, CNumeric, false},
	/** Integers */
	{"INT", TInteger, CNumeric, false},
	{"INTEGER", TInteger, CNumeric, false},
	{"TINYINT", TInteger, CNumeric, false},
	{"SMALLINT", TInteger, CNumeric, false},

	/* Large integers */
	{"BIGINT", TBigInt, CNumeric, false},
	{"IDENTIFIER", TBigInt, CNumeric, false},

	/* Floating point numbers */
	{"DOUBLE", TDouble, CNumeric, false},
	{"FLOAT", TFloat, CNumeric, false},
	{"REAL", TFloat, CNumeric, false},

	/* Date tyes */
	{"DATE", TDate, CDate, false},

	/* String types */
	{"CHAR", TCharN, CString, true},
	{"VARCHAR", TVarChar, CString, false},
	{"STR", TVarChar, CString, false},
	{"DSTRING", TDString, CString, false},

	/* Exact types */
	{"IPV4ADDR", TIPaddr, CExactMatch, false}
};
