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
// for Emacs -*- c++ -*-

#ifndef _WORKUNIT_IMP_H_
#define _WORKUNIT_IMP_H_

#include <string>

#include "ID.h"

class WorkUnitImp {

    protected:

        // this tells the system who owns this WorkUnit (in other words, the
        // wayPoint that this WorkUnit came from)
        WayPointID owner;

        // descriptin of the work going on. Please set this string with
        // descriptive strings
        std::string description;

    public:

        // constructor
        WorkUnitImp ();

        // destructor
        virtual ~WorkUnitImp ();

        // gets the owner... used for routing
        WayPointID &GetOwner ();

        // sets the owner
        void SetOwner (WayPointID &owner);

        // get the description of what this work unit does for the logging
        const char* GetDescription(void){ return description.c_str(); }

        // actually run the work that this guy needs to do
        virtual void Run();
};

#endif
