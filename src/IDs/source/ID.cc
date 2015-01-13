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
#include "ID.h"
#include "TwoWayList.cc"

#include <string>
#include <map>


// static member initialization

using namespace std;


std::atomic<size_t> IDUnique::nextID(1);

map<string, size_t> WayPointID::nameToID;
map<size_t, WayPointInfoImp::Info> WayPointInfoImp::infoMap;

map<string, size_t> TableScanID::nameToID;
InefficientMap<Size_t_key, TableScanInfoImp::Info> TableScanInfoImp::infoMap;
