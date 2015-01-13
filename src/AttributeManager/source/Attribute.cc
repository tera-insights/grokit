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
#include "Attribute.h"

using namespace std;

Attribute::Attribute():
    haveName(false), haveType(false), haveIndex(false), isPrimaryKey(false),
    isForeignKey(false) {
    }

Attribute::Attribute(int _index):
    index(_index), haveName(false), haveType(false), haveIndex(true),
    isPrimaryKey(false), isForeignKey(false){
    }

Attribute::Attribute(string _name):
    name(_name), haveName(true), haveType(false), haveIndex(false),
    isPrimaryKey(false), isForeignKey(false){
    }

Attribute::Attribute(string _name, string _type, Json::Value _jType, int _index,
        long int _uniques):
    haveName(true), haveType(true), haveIndex(true), isPrimaryKey(false),
    isForeignKey(false), haveUniques(true), name(_name), index(_index),
    uniques(_uniques), jType(_jType) {

        type = _type;
    }


string Attribute::GetName() const {

    // check if we have a name
    FATALIF(!haveName, "Attribute instance doesn't have a name!");

    // return
    return(name);
}

bool Attribute::HasName() const {
    return haveName;
}

void Attribute::SetName(string _name) {
    name = _name;
    haveName = true;
}

string Attribute::GetType() const {

    // check if we have a type
    FATALIF(!haveType, "Attribute instance doesn't have a name!");

    return type;
}

void Attribute::SetType(string &in) {
    type=in;
    haveType = true;
}

Json::Value Attribute::GetJType() const {
    FATALIF(!haveType, "Attribute instance doesn't have a type!");

    return jType;
}

void Attribute::SetJType(Json::Value &in) {
    jType = in;
    haveType = true;
}

int Attribute::GetIndex() {

    // check if we have an index
    FATALIF(!haveIndex, "Attribute instance doesn't have an index!");

    // return
    return(index);
}

void Attribute::SetIndex(int _index) {
    index = _index;
    haveIndex = true;
}

long int Attribute::GetUniques() {

    // SS: Commented for now, we care about statistics later
    // check if we have the uniques
    //FATALIF(!haveUniques, "Attribute instance doesn't have uniques!");

    // return
    return(uniques);
}

void Attribute::SetUniques(long int _uniques) {

    haveUniques = true;
    uniques = _uniques;
}

bool Attribute::IsPrimaryKey() {
    return(isPrimaryKey);
}

void Attribute::SetPrimaryKey(bool val) {
    isPrimaryKey = val;
}

bool Attribute::IsForeignKey() {
    return(isForeignKey);
}

string Attribute::GetForeignRelation() {

    // check if we have a foreign key here
    FATALIF(!isForeignKey, "Attribute is not a foreign key!");

    // return
    return(foreignRel);
}

string Attribute::GetForeignAttribute() {

    // check if we have a foreign key
    FATALIF(!isForeignKey, "Attribute is not a foreign key!");

    // return
    return(foreignAtt);
}

void Attribute::SetForeignKey(string _rel, string _att) {

    isForeignKey = true;
    foreignRel = _rel;
    foreignAtt = _att;
}

void Attribute::CopyTo(Attribute &out) const {
    out.haveName = haveName;
    out.name = name;

    out.haveType = haveType;
    out.type=type;

    out.haveIndex = haveIndex;
    out.index = index;

    out.haveUniques = haveUniques;
    out.uniques = uniques;

    out.isPrimaryKey = isPrimaryKey;

    out.isForeignKey = isForeignKey;
    out.foreignRel = foreignRel;
    out.foreignAtt = foreignAtt;

    out.jType = jType;
}

void Attribute::swap(Attribute &with) {

    // use the CopyTo method to swap.
    Attribute dummy;
    CopyTo(dummy);
    with.CopyTo(*this);
    dummy.CopyTo(with);
}

void Attribute::Print() {

    char idx[4];
    if (haveIndex)
        sprintf(idx, "%i", index);

    PDEBUG("[%s, %s, %s, %s, %s, %s]",
            haveName?name.c_str():"<no name>",
            haveType?type.c_str():"<no type>",
            haveIndex?idx:"<no index>",
            isPrimaryKey?"<is a primary key>":"<is NOT a primary key>",
            isForeignKey?foreignRel.c_str():"<not foreign key>",
            isForeignKey?foreignAtt.c_str():"<not foreign key>");
}

void Attribute :: toJson(Json::Value & dest) const {
    if( haveName )
        dest["name"] = name;
    if( haveType )
        dest["type"] = jType;
}

void Attribute :: fromJson(const Json::Value & src) {
    if( haveName = src.isMember("name") )
        name = src["name"].asString();

    if( haveType = src.isMember("type") )
        jType = src["type"];
}

void ToJson( const Attribute & src, Json::Value & dest ) {
    src.toJson(dest);
}

void FromJson( const Json::Value & src, Attribute & dest ) {
    dest.fromJson(src);
}
