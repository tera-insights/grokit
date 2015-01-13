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
#include "AttributeType.h"
#include "Swap.h"

#include <strings.h>


AttributeType AttributeType::CreateFromName(const char* name){
	// since we have relatively small number of types, we can aford to do a linear
	// search through the list of attribute properties

	// we want to allow case independence and the posibility that a size is associated
	// with the type
	int dataTypeInfoSize=sizeof(dataTypeInfo)/sizeof(DataTypeEl);
	for (int i=0; i<dataTypeInfoSize; i++){
		DataTypeEl& t = dataTypeInfo[i];
		// do we have a match?
		int tSz=strlen(t.name);
		if (strncasecmp(name, t.name,tSz )==0){
			// we found our match
			// do we need size as well?
			if (t.hasSize){
				// get the size
				int size;
				// make sure that the name does not end too soon
				if (name[tSz]==0 || sscanf(&(name[tSz+1]), "%d", &size)!=1){
					WARNING("Type %s has a size but one not found in %s.", t.name, name);
					return AttributeType();
				}
				// everything is fine, create the type using the private constructor
				return AttributeType(t.tType, t.tCat, size);
			} else {
				return AttributeType(t.tType, t.tCat);
			}
		}
	}

	// if we got here, we did not match any type
	WARNING("Type %s not recognized.", name);
	return AttributeType();
}


void AttributeType::swap(AttributeType& other){
	SWAP_STD(type, other.type);
	SWAP_STD(cat, other.cat);
	SWAP_STD(size, other.size);
}

void AttributeType::copy(AttributeType& other){
	type=other.type;
	cat=other.cat;
	size=other.size;
}

const char* AttributeType::getAttName(void){
	// go through the array and get the first name we encounter
	int dataTypeInfoSize=sizeof(dataTypeInfo)/sizeof(DataTypeEl);
	for (int i=0; i<dataTypeInfoSize; i++){
		DataTypeEl& t = dataTypeInfo[i];
		if (t.tType==type)
			return t.name;
	}
	// this should never happen
	assert(false);
}

void AttributeType::GetTypeAsStringCPP(char* where){
	if (size==-1){
		sprintf(where, "%s", getAttName());
	} else {
		sprintf(where, "%s<%d>", getAttName(), size);
	}
}

void AttributeType::GetTypeAsStringSQL(char* where){
	if (size==-1){
		sprintf(where, "%s", getAttName());
	} else {
		sprintf(where, "%s(%d)", getAttName(), size);
	}
}

// Override global swap
void swap( AttributeType & a, AttributeType & b ) {
    a.swap(b);
}
