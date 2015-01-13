// Copyright 2013 Tera Insights, LLC. All Rights Reserved.

// This header contains general function definitions for functions that serialize
// and deserialize various basic types to and from JSON

#ifndef _SERIALIZE_JSON_H_
#define _SERIALIZE_JSON_H_

#include <string>
#include <vector>
#include <map>
#include <set>
#include <type_traits>
#include <utility>

#include <cinttypes>
#include <exception>
#include <stdexcept>
#include <cstring>
#include <ctime>
#include <cmath>

#include "json.h"
#include "Errors.h"

// Specialize for basic types

// int
inline
void ToJson( const int & src, Json::Value & dest ) {
    dest = (Json::Int) src;
}

inline
void FromJson( const Json::Value & src, int & dest ) {
    dest = (int) src.asInt();
}

// unsigned int
inline
void ToJson( const unsigned int & src, Json::Value & dest ) {
    dest = (Json::UInt) src;
}

inline
void FromJson( const Json::Value & src, unsigned int & dest ) {
    dest = (unsigned int) src.asUInt();
}

// long
inline
void ToJson( const long& src, Json::Value & dest ) {
    dest = (Json::Int64) src;
}

inline
void FromJson( const Json::Value & src, long & dest ) {
    dest = (long) src.asInt64();
}

// unsigned long
inline
void ToJson( const unsigned long & src, Json::Value & dest ) {
    dest = (Json::UInt64) src;
}

inline
void FromJson( const Json::Value & src, unsigned long & dest ) {
    dest = (unsigned long) src.asUInt64();
}

// float
inline
void ToJson( const float & src, Json::Value & dest ) {
    dest = src;
}

inline
void FromJson( const Json::Value & src, float & dest ) {
    dest = src.asFloat();
}

// double
inline
void ToJson( const double & src, Json::Value & dest ) {
    dest = src;
}

inline
void FromJson( const Json::Value & src, double & dest ) {
    dest = src.asDouble();
}

// char *
inline
void ToJson( const char * & src, Json::Value & dest ) {
    dest = Json::Value(src);
}

inline
void FromJson(const Json::Value & src, char * & dest ) {
    dest = strdup(src.asCString());
}

// char
inline
void ToJson( const char & src, Json::Value & dest ) {
    dest = Json::Value((Json::UInt) src);
}

inline
void FromJson( const Json::Value & src, char & dest ) {
    dest = (char) src.asUInt();
}

// std::string
inline
void ToJson( const std::string & src, Json::Value & dest ) {
    dest = Json::Value(src);
}

inline
void FromJson( const Json::Value & src, std::string & dest ) {
    dest = src.asString();
}

// bool
inline
void ToJson( const bool & src, Json::Value & dest ) {
    dest = src;
}

inline
void FromJson( const Json::Value & src, bool & dest ) {
    dest = src.asBool();
}

// Json
inline
void ToJson( const Json::Value & src, Json::Value & dest ) {
    dest = src;
}

inline
void FromJson( const Json::Value & src, Json::Value & dest ) {
    dest = src;
}

/********* Overloads for container types *********/

// pair
template< class T1, class T2 >
void ToJson( const std::pair<T1, T2>& src, Json::Value & dest ) {
    dest = Json::Value(Json::arrayValue);
    ToJson(src.first, dest[0u]);
    ToJson(src.second, dest[1u]);
}

template< class T1, class T2 >
void FromJson( const Json::Value & src, std::pair<T1, T2> & dest ) {
    if( ! src.isArray() ) {
        throw new std::invalid_argument( "Attempting to build pair from non-array JSON");
    }

    if( ! (src.size() == 2) ) {
        throw new std::invalid_argument( "Attempting to build pair from array of invalid length");
    }

    FromJson(src[0u], dest.first);
    FromJson(src[1u], dest.second);
}

// std::vector
template< class T >
void ToJson( const std::vector<T> & src, Json::Value & dest ) {
    dest = Json::Value(Json::arrayValue);

    for( const T & el : src ) {
        Json::Value tmp;
        ToJson(el, tmp);
        dest.append(tmp);
    }
}

template< class T >
void FromJson( const Json::Value & src, std::vector<T> & dest ) {
    if( ! src.isArray() ) {
        throw new std::invalid_argument( "Attempting to build std::vector from non-array JSON" );
    }

    dest.clear();

    for( const Json::Value & el : src ) {
        T tmp;
        FromJson(el, tmp);
        dest.push_back(tmp);
    }
}

// std::set
template< class T >
void ToJson( const std::set<T> & src, Json::Value & dest ) {
    dest = Json::Value(Json::arrayValue);

    for( const T & el : src ) {
        Json::Value tmp;
        ToJson(el, tmp);
        dest.append(tmp);
    }
}

template< class T >
void FromJson( const Json::Value & src, std::set<T> & dest ) {
    if( ! src.isArray() ) {
        throw new std::invalid_argument( "Attempting to build std::set from non-array JSON" );
    }

    dest.clear();

    for( const Json::Value & el : src ) {
        T tmp;
        FromJson(el, tmp);
        dest.push_back(tmp);
    }
}

// std::map

// Special case for string keys
template< class V >
void ToJson( const std::map<std::string, V> & src, Json::Value & dest ) {
    dest = Json::Value(Json::objectValue);
    for( const auto & el : src ) {
        ToJson(el.second, dest[el.first]);
    }
}

template< class V >
void FromJson( const Json::Value & src, std::map<std::string, V> & dest ) {
    if( ! src.isObject() ) {
        throw new std::invalid_argument("Attempting to build std::map with string keys from non-object JSON");
    }

    dest.clear();

    auto members = src.getMemberNames();
    for( const std::string & name : members ) {
        FromJson(src[name], dest[name]);
    }
}

// General case
template< class K, class V >
void ToJson( const std::map<K, V> & src, Json::Value & dest ) {
    dest = Json::Value(Json::arrayValue);

    for( const auto & it : src ) {
        Json::Value tmp(Json::arrayValue);

        ToJson(it.first, tmp[0u]);
        ToJson(it.second, tmp[1u]);

        dest.append(tmp);
    }
}

template< class K, class V >
void FromJson( const Json::Value & src, std::map<K, V> & dest ) {
    if( ! src.isArray() ) {
        throw new std::invalid_argument( "Attempting to build std::map with non-string keys from non-array JSON" );
    }

    dest.clear();

    for( const Json::Value & el : src ) {
        if( ! el.isArray() ) {
            throw new std::invalid_argument( "Invalid element while building std::map with non-string keys from JSON" );
        }

        if( !(el.size() == 2) ) {
            std::string errMsg = "Expected exactly 2 values in element while building std::map with non-string keys from JSON, got ";
            errMsg += std::to_string(el.size());
            throw new std::invalid_argument( errMsg );
        }

        K tmpKey;
        V tmpVal;

        FromJson(el[0u], tmpKey);
        FromJson(el[1u], tmpVal);

        dest[tmpKey] = tmpVal;
    }
}

// timespec
inline
void ToJson( const timespec & src, Json::Value & dest ) {
    dest = Json::Value(Json::objectValue);
    dest["sec"] =  static_cast<Json::Int64>(src.tv_sec);
    dest["nsec"] = static_cast<Json::Int64>(src.tv_nsec);
}

inline
void FromJson( const Json::Value & src, timespec & dest ) {
    if( ! src.isObject() ) {
        throw new std::invalid_argument( "Attempting to build timespec from non-object JSON");
    }

    if(  (!src.isMember("sec")) || (!src.isMember("nsec")) ) {
        throw new std::invalid_argument( "Attempting to build timespec from invalid JSON object");
    }

    dest.tv_sec = src["sec"].asInt64();
    dest.tv_nsec = src["nsec"].asInt64();
}

#endif // _SERIALIZE_JSON_H_
