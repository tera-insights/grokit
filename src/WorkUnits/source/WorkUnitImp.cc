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

#include "WorkUnitImp.h"

#include <iostream>

WayPointID &WorkUnitImp :: GetOwner () {
	return owner;
}

void WorkUnitImp :: SetOwner (WayPointID &newOwner) {
	newOwner.swap (owner);
}

WorkUnitImp :: WorkUnitImp () {
}

WorkUnitImp :: ~WorkUnitImp () {
}

void WorkUnitImp :: Run () {
	cerr << "This is quite strange... you have tried to run a ";
	cerr << "generic work unit that cannot be run.\n";
}
