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
#ifndef _SLOT_ID_H
#define _SLOT_ID_H

#include <sstream>
#include <stdexcept>

#include "Config.h"
#include "IDInt.h"
#include "Swap.h"
#include "TwoWayList.h"
#include "TwoWayList.cc"
#include "EfficientMap.h"
#include "EfficientMap.cc"
#include "SerializeJson.h"


/** Class to implement a slot id. The slots are used to reffer to
 * columns in a chunk and to columns on disk.
 */

class SlotID : public IDInt {
    public:
        // we just neeed to redefine the constructor from int
        SlotID(size_t val);

        // copy constructor
        SlotID(const SlotID& other);

        // default constructor
        SlotID(){ }

        //destructor
        virtual ~SlotID() {}

        // assignment
        SlotID& operator = (const SlotID& other);

        // swap for containers
        void swap (SlotID &other);
        void copy(const SlotID& other);
        // the part inherited from IDInt should provide all the remaining functionality

        void getInfo(IDInfo& where) const;

        void toJson( Json::Value & dest ) const;
        void fromJson( const Json::Value & src );
};

/** Implementation of corresponding Info class */
class SlotInfoImp : public IDInfoImp{
    private:
        size_t id; // the numeric id so we can recreate the

    public:
        SlotInfoImp(size_t _id);

        virtual std::string getIDAsString() const OVERRIDE_SPEC;
        virtual std::string getName() const OVERRIDE_SPEC;

        virtual ~SlotInfoImp(){ }
};

/** Slot pairs are used to map logical to physical columns

  struct since it is just a container for two slots
  */

struct SlotPair {
    SlotID first;
    SlotID second;

    // default constructor
    SlotPair(){}

    // regular consructor
    SlotPair(SlotID _first, SlotID _second):
        first(_first),second(_second){}

    void swap(SlotPair& other);
    void copy(const SlotPair& other);
    void Print() {
        IDInfo f;
        first.getInfo(f);
        IDInfo s;
        second.getInfo(s);
        std::cout << "\nFirst ID = " << (f.getIDAsString()).c_str() << " Second ID = " << (s.getIDAsString()).c_str();
    }

    void toJson( Json::Value & ) const;
    void fromJson( const Json :: Value & );
};

/** interface class for SlotInfoImp */
class SlotInfo : public IDInfo{
    public:
        // we just need a constructor
        SlotInfo(size_t _id);

        //and destructor
        virtual ~SlotInfo() {}
};


// container used to keep track of slots
typedef TwoWayList< SlotID > SlotContainer;

// container for pairs of slots
typedef TwoWayList< SlotPair > SlotPairContainer;

// map between two ints
typedef EfficientMap < SlotID, SlotID> SlotToSlotMap;

/////////////////
// INLINE METHODS

inline
void SlotID :: toJson( Json::Value & dest ) const {
    dest = static_cast<Json::Int64>(id);
}

inline
void ToJson( const SlotID & src, Json::Value & dest ) {
    src.toJson(dest);
}

inline
void SlotID :: fromJson( const Json::Value & src ) {
    id = static_cast<size_t>(src.asInt64());
}

inline
void FromJson( const Json::Value & src, SlotID & dest ) {
    dest.fromJson(src);
}

inline
void SlotID::copy(const SlotID& other){
    id=other.id;
}

inline
void SlotID::getInfo(IDInfo& where) const{
    SlotInfo ret(id);
    where.swap(ret);
}

inline
SlotID::SlotID(size_t val):IDInt(val){ }

inline
SlotID::SlotID(const SlotID& other):IDInt(other.id){}

inline
SlotID& SlotID::operator = (const SlotID& other){
    id=other.id;
    return (*this);
}

inline
void SlotID::swap (SlotID &other){
    SWAP_STD(id, other.id);
}

// Override swap
inline
void swap( SlotID & a, SlotID & b ) {
    a.swap(b);
}

inline
SlotInfoImp::SlotInfoImp(size_t _id): id(_id){ }

inline
std::string SlotInfoImp::getIDAsString() const {
    std::stringstream s;
    std::string rez;
    if (id == ID_UNINITIALIZED){
        rez = "Uninitialized";
    } else {
        s << id ;
        s >> rez;
    }
    return rez;
}

// the name is a concatenation of Slot and id
inline
std::string SlotInfoImp::getName() const {
    std::stringstream s;
    std::string rez;
    if (id == ID_UNINITIALIZED){
        rez = "Uninitialized";
    } else {
        s << "Slot_" << id ;
        s >> rez;
    }
    return rez;
}

inline
SlotInfo::SlotInfo(size_t _id){
    info = new SlotInfoImp(_id);
}

inline void SlotPair::swap(SlotPair& other){
    SWAP_STD(first, other.first);
    SWAP_STD(second, other.second);
}

// Override global swap
inline
void swap( SlotPair & a, SlotPair & b ) {
    a.swap(b);
}

inline void SlotPair::copy(const SlotPair& other) {
    first = other.first;
    second = other.second;
}

inline
void SlotPair :: toJson( Json::Value & dest ) const {
    dest = Json::Value(Json::arrayValue);
    ToJson(first, dest[0u]);
    ToJson(second, dest[1u]);
}

inline
void ToJson( const SlotPair & src, Json::Value & dest ) {
    src.toJson(dest);
}

inline
void SlotPair :: fromJson( const Json::Value & src ) {
    if( ! src.isArray() || src.size() < 2 )
        throw new std::runtime_error("Attempting to read SlotPair from invalid JSON");

    FromJson(src[0u], first);
    FromJson(src[1u], second);
}

inline
void FromJson( const Json::Value & src, SlotPair & dest ) {
    dest.fromJson(src);
}

#endif // _SLOT_ID_H
