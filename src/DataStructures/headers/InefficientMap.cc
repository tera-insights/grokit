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

#ifndef _INEFFICIENT_MAP_C
#define _INEFFICIENT_MAP_C

#include "InefficientMap.h"

#include <stdlib.h>
#include <iostream>

template <class Key, class Data> void
InefficientMap <Key, Data> :: SuckUp (InefficientMap &suckMe) {
    container.MoveToFinish ();
    suckMe.container.MoveToStart ();
    container.SwapRights (suckMe.container);
}

template <class Key, class Data> void
InefficientMap <Key, Data> :: swap (InefficientMap &withMe) {
    container.swap (withMe.container);
}

template <class Key, class Data> void
InefficientMap <Key, Data> :: Insert (Key &insertKey, Data &insertData) {

    Node foo;
    foo.key.swap (insertKey);
    foo.data.swap (insertData);
    container.Insert (foo);
}

template <class Key, class Data> Data &
InefficientMap <Key, Data> :: Find (const Key &findMe) {

    static Data garbage;
    int numRight = container.RightLength ();

    while (container.RightLength () != 0) {
        if (container.Current().key.IsEqual (findMe)) {
            return container.Current().data;
        }
        container.Advance ();
    }

    container.MoveToStart ();
    while (container.RightLength () != numRight) {
        if (container.Current().key.IsEqual (findMe)) {
            return container.Current().data;
        }
        container.Advance ();
    }
    return garbage;
}

template <class Key, class Data> int
InefficientMap <Key, Data> :: IsThere (const Key &findMe) {

    static Data garbage;
    int numRight = container.RightLength ();

    while (container.RightLength () != 0) {
        if (container.Current().key.IsEqual (findMe)) {
            return 1;
        }
        container.Advance ();
    }

    container.MoveToStart ();
    while (container.RightLength () != numRight) {
        if (container.Current().key.IsEqual (findMe)) {
            return 1;
        }
        container.Advance ();
    }
    return 0;
}

template <class Key, class Data> void
InefficientMap <Key, Data> :: Clear(void){
    MoveToStart();
    while (!AtEnd()){
        Key key;
        Data data;
        Node foo;
        container.Remove(foo);
        key.swap(foo.key);
        data.swap(foo.data);
    }
}

template <class Key, class Data> int
InefficientMap <Key, Data> :: Remove (Key &findMe, Key &putKeyHere, Data &putDataHere) {

    int numRight = container.RightLength ();
    Node foo;

    while (container.RightLength () != 0) {
        if (container.Current().key.IsEqual (findMe)) {
            container.Remove (foo);
            putKeyHere.swap (foo.key);
            putDataHere.swap (foo.data);
            return 1;
        }
        container.Advance ();
    }

    container.MoveToStart ();
    while (container.RightLength () != numRight) {
        if (container.Current().key.IsEqual (findMe)) {
            container.Remove (foo);
            putKeyHere.swap (foo.key);
            putDataHere.swap (foo.data);
            return 1;
        }
        container.Advance ();
    }

    return 0;
}

template <class Key, class Data> Key&
InefficientMap <Key, Data> ::CurrentKey(){ return container.Current().key; }

template <class Key, class Data> Data&
InefficientMap <Key, Data> ::CurrentData(){ return container.Current().data; }

template <class Key, class Data> void
InefficientMap <Key, Data> ::Advance(){ container.Advance();}

template <class Key, class Data> void
InefficientMap <Key, Data> ::Retreat(){ container.Retreat(); }

template <class Key, class Data> void
InefficientMap <Key, Data> ::MoveToStart(){ container.MoveToStart(); }

template <class Key, class Data> void
InefficientMap <Key, Data> ::MoveToFinish(){ container.MoveToFinish(); }

template <class Key, class Data> bool
InefficientMap <Key, Data> ::AtStart() const { return container.AtStart(); }

template <class Key, class Data> bool
InefficientMap <Key, Data> ::AtEnd() const { return container.AtEnd(); }

template <class Key, class Data>
void
InefficientMap<Key, Data> :: toJson( Json::Value & dest ) const {
    dest = Json::Value(Json::arrayValue);
    FOREACH_TWL( el, container ) {
        Json::Value tmp;
        el.toJson(tmp);
        dest.append(tmp);
    } END_FOREACH;
}

template <class Key, class Data>
void
InefficientMap <Key, Data> :: fromJson( const Json::Value & src ) {
    if( ! src.isArray() ) {
        throw new std::invalid_argument("Tried to create InefficientMap from non-array JSON");
    }

    Clear();

    for( const Json::Value & el : src ) {
        Node tmp;
        tmp.fromJson(el);
        container.Insert(tmp);
    }
}

#endif

