//
//  Copyright 2012 Christopher Dudley
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
#include <string>
#include "ArgFormat.h"

using namespace std;

ArgFormat::ArgFormat( const string _type, const string _wrapper, bool _noExtract ) 
  : type(_type), wrapper(_wrapper), noExtract(_noExtract)
{ }

ArgFormat::ArgFormat( const ArgFormat & a ) {
  type = a.type;
  wrapper = a.wrapper;
  noExtract = a.noExtract;
}

ArgFormat::~ArgFormat() { }

string ArgFormat::format( const string & name ) const {
  size_t n = wrapper.find("%s");

  string prev = wrapper.substr(0, n);
  string after("");

  if( n+2 < wrapper.size() )
    after = wrapper.substr(n+2);

  return prev + name + after;
}

string ArgFormat::getType() const {
  return type;
}

bool ArgFormat::forceNoExtract() const {
  return noExtract;
}
