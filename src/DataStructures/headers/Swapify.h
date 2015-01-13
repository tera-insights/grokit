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

#ifndef _SWAPIFY_H_
#define _SWAPIFY_H_

#include "Swap.h"
#include "SerializeJson.h"

// The following templates are used to quickly and easily create a
// class that wraps around a simple type (such as an int) and can be
// put into a TwoWayList or an InefficientMap.  The "Swapify" template
// attaches only a swap operation (so a simple type can be used with
// a TwoWayList)... the "Keyify" template attaches both a swap and an
// IsEqual (so a simple type can be used with either the TwoWayList or
// the InefficientMap).  For example, the following is valid code, which
// uses the Keyify template to attach a swap and IsEqual to an int:
//
// void IntSwap (int &a, int &b) {
//         int temp;
//         temp = a;
//         a = b;
//         b = temp;
// }
//
// int IntCheck (int &a, int &b) {
//         return a == b;
// }
//
// int main () {
//
//      typedef Keyify <int, IntSwap, IntCheck> keyifiedInt;
//          InefficientMap <keyifiedInt, keyifiedInt> foo;
//          keyifiedInt bar1, bar2;
//
//          bar1 = 12;
//          bar2 = 43;
//          foo.Insert (bar1, bar2);
//          ...
//
// SPECIAL NOTE: ONLY USE THESE TEMPLATES WITH SIMPLE TYPES (ints, doubles, etc.).
// These templates maky use of the = operation, so they are only safe with such types.
// If the type is anything more complicated, then the thing to do is to create a proper
// class containing the swap and IsEqual operations!
//
template <class Type>
class Swapify {
private:
    Type data;

public:
    void swap (Swapify<Type> &withMe) {
        SWAP_STD(data, withMe.data);
    }

    void copy (Swapify<Type> &fromMe) {
        data = fromMe.data;
    }

    Swapify (const Type castFromMe) {
        data = castFromMe;
    }

    Type& GetData(void){
        return data;
    }

    operator Type() {
        return data;
    }

    Swapify () {}
    virtual ~Swapify () {}

    void toJson( Json::Value & dest ) const {
        ToJson(data, dest);
    }

    void fromJson( const Json::Value & src ) {
        FromJson(src, data);
    }
};

template< class Type >
void ToJson( const Swapify<Type> & src, Json::Value & dest ) {
    src.toJson(dest);
}

template< class Type >
void FromJson( const Json::Value & src, Swapify<Type> & dest ) {
    dest.fromJson(src);
}

// STL-compatible swap function
template< class Type>
void swap( Swapify<Type>& a, Swapify<Type>& b ) {
    a.swap(b);
}

template <class Type>
class Keyify {
private:
    Type data;

public:
    void swap (Keyify<Type> &withMe) {
        SWAP_STD(data, withMe.data);
    }


    Keyify (const Type & castFromMe) {
        data = castFromMe;
    }

    void copy (Keyify<Type> &fromMe) {
        data = fromMe.data;
    }

    operator Type() {
        return data;
    }

    bool operator < ( const Keyify<Type> & checkMe ) const {
        return data < checkMe.data;
    }

    bool operator == ( const Keyify<Type> & checkMe ) const {
        return data == checkMe.data;
    }

    int IsEqual(const Keyify<Type> &checkMe) const {
        return data == checkMe.data;
    }

    bool LessThan(const Keyify<Type>& withMe) const {
        return data < withMe.data;
    }

    Keyify () {}
    virtual ~Keyify () {}
};

// STL-compatible swap function
template< class Type >
void swap( Keyify<Type>& a, Keyify<Type>& b ) {
    a.swap(b);
}

#endif // _SWAPIFY_H_
