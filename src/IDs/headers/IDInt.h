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
#ifndef _ID_INT_H_
#define _ID_INT_H_

#include "IDInterface.h"

/** The class IDInt is the base class for all the id classes that need
  to be used as indexes in a vector. The focus in on efficiency.

  The main property of the classes derived form this base class is
  that the IDs do not need to be unique and an interface to produce
  a new unique id is not provided. If this is a desrirable propoerty,
  the id shuld inherit from IDUnique not from IDInt

  Classes derived from this class cannot be mixed into containers.

  The int inside the class allows virtually unlimited ids (2^64 on 64 bit architecture)

NOTE: we do not refine IDInfo or IDInfoImp for this class since they need to be
refined when the derived classes from IDInt are specified
*/

class IDInt : public IDInterface {
    protected:
        size_t id;

        //  protected operators to make sure they are not called except from derived classes
        /** copy constructor */
        IDInt(const IDInt& other);

        /** Operator = needed for some containers */
        IDInt& operator= (const IDInt &other);

    public:
        /** Constructor from size_t. This constructor is needed since there
         * is no way to create unique ids
         */
        IDInt(size_t val);

        /** Default constructor */
        IDInt();

        /** Destructor */
        virtual ~IDInt() {}

        /** Operator < for containers that need it */
        bool operator<(const IDInt& other) const;
        bool LessThan(const IDInt& other) const;

        /** Operator ==  for containers and other uses */
        bool operator==(const IDInt& other) const;

        // == for TwoWayList
        bool IsEqual(const IDInt& other) const{ return id == other.id; }

        // is the ID valid?
        bool IsValid(void){ return id != ID_UNINITIALIZED; }

        int GetInt() {return id;}

        size_t GetValue() const { return id; }

        /** conversion to int operator. Needed if the ID is used as an index in a vector */
        operator int();

        /** swap has to be implemented higher up */
};

///////////////////////////
/// INLINE METHODS


inline
IDInt::IDInt(size_t val){ id=val; }

inline
IDInt::IDInt(){
    // with this id, if the default constructed object is used as an index,
    // a segfault results immediatelly.
    id = ID_UNINITIALIZED;
}

inline
IDInt::IDInt(const IDInt& other):id(other.id){}

inline
bool IDInt::operator<(const IDInt& other) const{
    return (id<other.id);
}


inline
bool IDInt::LessThan(const IDInt& other) const{
    return (id<other.id);
}


inline
bool IDInt::operator==(const IDInt& other) const{
    return (id==other.id);
}

inline
IDInt& IDInt::operator= (const IDInt &other){
    id=other.id;
    return (*this);
}


inline
IDInt::operator int(){ return id; }

#endif // _ID_INT_H_
