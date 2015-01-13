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

#ifndef _EFFICIENT_MAP_C
#define _EFFICIENT_MAP_C

#include "EfficientMap.h"
#include "Swap.h"

#include <stdlib.h>
#include <iostream>
#include <stdexcept>

// basic constructor function
template <class Key, class Data>
EfficientMap <Key, Data> :: EfficientMap () :
    list(new Header)
{

    // set up the initial values for an empty list
    list->first = new Node (MAXLEVELS);
    list->last = new Node (MAXLEVELS);
    list->current = list->first;
    list->curDepth = 0;

    // note that the size of the list at every level is zero
    for (int i = 0; i < MAXLEVELS; i++) {
        list->first->next[i] = list->last;
    }

    list->last->previous = list->first;
}

template <class Key, class Data>
EfficientMap <Key, Data> :: EfficientMap( EfficientMap<Key, Data> & other ) : EfficientMap() {
    copy(other);
}

template <class Key, class Data>
EfficientMap<Key, Data> & EfficientMap<Key, Data> :: operator =( EfficientMap<Key, Data> & other ) {
    copy(other);
}

// basic deconstructor function
template <class Key, class Data>
EfficientMap <Key, Data> :: ~EfficientMap ()
{

    while (list->first != list->last) {
        list->first = list->first->next[0];
        delete list->first->previous;
    }
    delete list->first;

    // kill the header
    delete list;
}

// swap operator
template <class Key, class Data> void
EfficientMap <Key, Data> :: swap (EfficientMap<Key, Data> & List)
{

    Header *temp = List.list;
    List.list = list;
    list = temp;

}

// make the first node the current node
template <class Key, class Data> void
EfficientMap <Key, Data> :: MoveToStart ()
{
    list->current = list->first;
}

template <class Key, class Data> bool
EfficientMap <Key, Data> :: AtStart () const
{
    return (list->current == list->first);
}

// make the first node the current node
template <class Key, class Data> void
EfficientMap <Key, Data> :: MoveToFinish ()
{
    list->current = list->last->previous;
}

template <class Key, class Data> void
EfficientMap <Key, Data> :: Advance ()
{
    list->current = list->current->next[0];
}

// move backwards through the list
template <class Key, class Data> void
EfficientMap <Key, Data> :: Retreat ()
{
    list->current = list->current->previous;
}

// move forwards through the list
template <class Key, class Data> void
EfficientMap <Key, Data> :: Advance (int whichLevel)
{
    list->current = list->current->next[whichLevel];
}

// insert an item at the current poition
template <class Key, class Data> void
EfficientMap <Key, Data> :: Insert (Node *temp, int whichLevel)
{

    Node *left = list->current;
    Node *right = list->current->next[whichLevel];

    left->next[whichLevel] = temp;
    temp->next[whichLevel] = right;

    if (whichLevel == 0) {
        temp->previous = left;
        right->previous = temp;
    }

}

template <class Key, class Data> bool
EfficientMap <Key, Data> :: AtEnd () const {
    return (list->current->next[0] == list->last);
}

template <class Key, class Data> int
EfficientMap <Key, Data> :: AtEnd (int whichLevel) const {
    return (list->current->next[whichLevel] == list->last);
}

template <class Key, class Data> void
EfficientMap <Key, Data> :: Clear(void){
    MoveToStart();
    while( ! AtEnd() ) {
        Key& curKey = CurrentKey();

        Key tmpKey;
        Data tmpData;

        Remove(curKey, tmpKey, tmpData);
        MoveToStart();
    }
}

template <class Key, class Data> int
EfficientMap <Key, Data> :: Remove (const Key &key, Key &removedKey, Data &removedData) {

    MoveToStart ();

    // start at the highest level and work down
    for (int i = list->curDepth - 1; i >= 0; i--) {

        // find the location to insert at this level
        while (1) {

            // keep looping until either we reach the end
            if (AtEnd (i))
                break;

            // or we find a larger item
            if (key < (CurrentKey (i)))
                break;

            // see if we actually found it
            if (key == (CurrentKey (i))) {
                Node *temp;
                Remove (temp, i);

                // if this is the lowest level, then kill the node
                if (i == 0) {
                    temp->data.swap (removedData);
                    temp->key.swap (removedKey);
                    delete temp;
                    return 1;
                }

                break;
            }

            // if we made it here, we have more data to loop thru
            Advance (i);
        }

    }

    return 0;
}

template <class Key, class Data> Data &
EfficientMap <Key, Data> :: Find (const Key &key) {

    MoveToStart ();

    // start at the highest level and work down
    for (int i = list->curDepth - 1; i >= 0; i--) {

        // find the location to go down from this level
        while (1) {

            // keep looping until either we reach the end
            if (AtEnd (i))
                break;

            // see if we actually found it
            if (key == (CurrentKey (i)) && i == 0)
                return CurrentData (0);

            // or we find a larger item
            if (key < (CurrentKey (i)) || key == (CurrentKey (i)))
                break;

            // if we made it here, we have more data to loop thru
            Advance (i);
        }

    }

    // if we made it here, we did not find it
    throw std::out_of_range("Attempted to access data with undefined key");
}

template <class Key, class Data> int
EfficientMap <Key, Data> :: IsThere (const Key &key) {

    MoveToStart ();

    // start at the highest level and work down
    for (int i = list->curDepth - 1; i >= 0; i--) {

        // find the location to go down from this level
        while (1) {

            // keep looping until either we reach the end
            if (AtEnd (i))
                break;

            // see if we actually found it
            if (key == (CurrentKey (i)) && i == 0)
                return 1;

            // or we find a larger item
            if (key < (CurrentKey (i)) || key == (CurrentKey (i)))
                break;

            // if we made it here, we have more data to loop thru
            Advance (i);
        }

    }

    // if we made it here, we did not find it
    return 0;
}

    template <class Key, class Data> void
EfficientMap <Key, Data> :: Insert (Key &key, Data &data)
{

    MoveToStart ();

    // first, we figue out how many levels are in the new node
    int numLevels = 1;
    while (drand48 () > 0.5) {
        numLevels++;
        if (numLevels == MAXLEVELS) {
            numLevels--;
            break;
        }
    }

    // now create the node
    Node *temp = new Node (numLevels);
    temp->key.swap (key);
    temp->data.swap (data);

    // now, see how many levels we must work thru
    MoveToStart ();
    if (list->curDepth < numLevels)
        list->curDepth = numLevels;

    // actually do the insertion
    for (int i = list->curDepth - 1; i >= 0; i--) {

        // find the location to insert at this level
        while (1) {

            // keep looping until either we reach the end
            if (AtEnd (i)) {
                break;
            }


            // or we find a larger item
            if (! (CurrentKey(i) < (temp->key)) )
                break;

            // if we made it here, we have more data
            Advance (i);
        }

        // do the insertion, if we are far enough down
        if (i < numLevels) {
            Insert (temp, i);
        }
    }
}

template <class Key, class Data> void
EfficientMap <Key, Data> :: SuckUp(EfficientMap <Key, Data>& other){

    // scan the other map and insert one element at the time
    for (other.MoveToStart(); !other.AtEnd(); other.Advance())
        Insert(other.CurrentKey(), other.CurrentData());

}


template <class Key, class Data> void
EfficientMap <Key, Data> :: Print(){
    std::cerr << "MAP: ";
    for (MoveToStart(); !AtEnd(); Advance()){
        std::cerr << CurrentKey() << "->" << CurrentData() << " ";
    }
    std::cerr << std::endl;
}

template <class Key, class Data> void
EfficientMap <Key, Data> :: copy(EfficientMap <Key, Data>& other){

    // clean up our content
    EfficientMap empty;
    this->swap(empty);

    // scan the other map and insert one element at the time
    for (other.MoveToStart(); !other.AtEnd(); other.Advance()){
        Key myKey;
        myKey.copy(other.CurrentKey());
        Data myData;
        myData.copy(other.CurrentData());

        Insert(myKey, myData);
    }

}



template <class Key, class Data> Data &
EfficientMap <Key, Data> :: CurrentData () const
{
    return list->current->next[0]->data;
}

template <class Key, class Data> Key &
EfficientMap <Key, Data> :: CurrentKey () const
{
    return list->current->next[0]->key;
}

template <class Key, class Data> Data &
EfficientMap <Key, Data> :: CurrentData (int whichLevel) const
{
    return list->current->next[whichLevel]->data;
}

template <class Key, class Data> Key &
EfficientMap <Key, Data> :: CurrentKey (int whichLevel) const
{
    return list->current->next[whichLevel]->key;
}

// remove an item from the current poition
template <class Key, class Data> void
EfficientMap <Key, Data> :: Remove (Node *&removeMe, int whichLevel)
{
    removeMe = list->current->next[whichLevel];
    list->current->next[whichLevel] = removeMe->next[whichLevel];

    if (whichLevel == 0)
        removeMe->next[whichLevel]->previous = list->current;
}

template <class Key, class Data>
void EfficientMap <Key, Data> :: toJson( Json::Value & dest ) const {
    dest = Json::Value(Json::arrayValue);

    // list->first is a guard node. The real first node is the one after it.
    // Same thing with list->last. When we've reached that node, we're done.
    // An empty map has list->first->next[0] = list->last
    for( Node * curVal = list->first->next[0]; curVal != list->last; curVal = curVal->next[0] ) {
        Json::Value tmp(Json::arrayValue);

        ToJson(curVal->key, tmp[0u]);
        ToJson(curVal->data, tmp[1u]);

        dest.append(tmp);
    }
}

template <class Key, class Data>
void EfficientMap <Key, Data> :: fromJson( const Json::Value & src ) {
    if( ! src.isArray() ) {
        throw new std::invalid_argument("Tried to create EfficientMap from non-array JSON");
    }

    Clear();

    for( const Json::Value & el : src ) {
        if( ! el.isArray() ) {
            throw new std::invalid_argument( "Invalid element while building EfficientMap from JSON" );
        }

        if( !(el.size() == 2) ) {
            std::string errMsg = "Expected exactly 2 values in element while building EfficientMap from JSON, got ";
            errMsg += std::to_string(el.size());
            throw new std::invalid_argument( errMsg );
        }

        Key tmpKey;
        Data tmpData;

        FromJson(el[0u], tmpKey);
        FromJson(el[1u], tmpData);

        Insert( tmpKey, tmpData );
    }
}

#endif

