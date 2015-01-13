#include "Dictionary.h"
#include "Errors.h"

#include <list>

#include <iostream>
#include <limits>
#include <utility>

using namespace std;

// Static initialization
Dictionary::DictionaryMap Dictionary::dicts;

// Constructor
Dictionary :: Dictionary( void ) :
    indexMap(),
    reverseMap(),
    orderMap(),
    nextID(0),
    modified(false),
    orderValid(false)
{ }

Dictionary :: Dictionary( const std::string name ) :
    indexMap(), reverseMap(), orderMap(),
    nextID(0), modified(false), orderValid(false)
{
    Load(name.c_str());
}

// Destructor
Dictionary :: ~Dictionary( void ) {
}

const char * Dictionary :: Dereference( IntType id ) const {
    IndexMap::const_iterator it = indexMap.find( id );
    if( it != indexMap.end() )
        return it->second.c_str();
    else {
      return "NULL";
    }
}

Dictionary::IntType Dictionary :: Lookup( const StringType& s, const IntType invalid ) const {
    ReverseMap::const_iterator it = reverseMap.find( s );
    if( it != reverseMap.end() )
        return it->second;
    else
        return invalid;
}

Dictionary::IntType Dictionary :: Lookup( const char * str, const IntType invalid ) const {
    StringType s(str);
    return Lookup( s, invalid );
}

Dictionary::IntType Dictionary :: Insert( StringType& s ) {
    modified = true;
    orderValid = false;

    indexMap[nextID] = s;
    reverseMap[s] = nextID;

    return nextID++;
}

Dictionary::IntType Dictionary :: Insert( const char * str, const IntType maxID ) {
    FATALIF( nextID > maxID, "Error: Unable to add new value [%s] to dictionary."
        " Next ID %u greater than specified maximum ID %u.", str, nextID, maxID );

    StringType s(str);
    return Insert( s );
}

void Dictionary :: Integrate( Dictionary& other, TranslationTable& trans ) {
    for( auto el : other.indexMap ) {
        IntType id = el.first;

        StringType str = el.second;
        auto revit = reverseMap.find(str);
        if( revit == reverseMap.end() ) {
            // This string isn't in my map.
            IntType myID = Insert( str );
            trans[id] = myID;
        } else {
            IntType myID = revit->second;
            if( myID != id ) {
                // The string is in my dictionary, but the IDs are different.
                trans[id] = myID;
            }
        }

        // Otherwise, both IDs are the same and we need no insertions or
        // translations
    }

    // Recompute sorted order
    ComputeOrder();
}

void Dictionary :: ComputeOrder( void ) {
    if( !orderValid ) {
        list<StringType> sortList;
        for( ReverseMap::const_iterator it = reverseMap.begin(); it != reverseMap.end(); ++it ) {
            const StringType& str = it->first;
            sortList.push_back(str);
        }

        sortList.sort();

        IntType curIndex = sortList.size() - 1;
        for( list<StringType>::const_reverse_iterator it = sortList.rbegin(); it != sortList.rend(); ++it ) {
            const StringType& str = *it;
            IntType ID = reverseMap[str];

            orderMap[ID] = curIndex--;
        }

        orderValid = true;
    }
}

Dictionary::DiffType Dictionary::Compare( IntType firstID, IntType secondID ) const {
    return orderMap.at(firstID) - orderMap.at(secondID);
}

Dictionary::const_iterator Dictionary::cbegin( void ) const {
    return indexMap.cbegin();
}

Dictionary::const_iterator Dictionary::cend( void ) const {
    return indexMap.cend();
}

Dictionary::const_iterator Dictionary::begin( void ) const {
    return indexMap.cbegin();
}

Dictionary::const_iterator Dictionary::end( void ) const {
    return indexMap.cend();
}

Dictionary & Dictionary :: GetDictionary( const std::string name ) {
    if( dicts.find(name) == dicts.end() ) {
        dicts.insert( make_pair(name, Dictionary(name)) );
    }

    return dicts[name];
}
