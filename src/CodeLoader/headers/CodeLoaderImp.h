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

#ifndef _CODELOADERIMP_H
#define _CODELOADERIMP_H

#include <iostream>
#include <fstream>
#include <map>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>
#include <sys/time.h>
#include <dlfcn.h>

#include "EventProcessor.h"
#include "EventProcessorImp.h"
#include "MessageMacros.h"
#include "Message.h"
#include "CLMessages.h"
#include "WayPointConfigureData.h"

/** This is the code loader. It is a stateless class, as it does not
 * save any information about previously loaded code -- that
 * information spreads through the system in the form of Function
 * objects. The functionality it provides is quite simple. All it does
 * is take a collection of symbolic configuration objects, loads them in the system
 * as a dynamically linked library, and gives back the collection of functions and
 * modules in the form of waypoing configuration messages.
 *
 * The code loader is implemented as an event processor with only
 * one input message --a request for new code to be loaded-- and one output
 * message with the code.
 **/
class CodeLoaderImp: public EventProcessorImp {
    private:
        const char* srcDir; // the source directory
        EventProcessor coordinator; // the coordinator (gets the message with the code)

        // process a new request for code to be loaded!
        // the code loader chases down the NULL functions and loads the
        // correct functions from the library
        // configMessages is both an input and output argument
        void Load(std::string dirName, WayPointConfigurationList &configMessages);

        // map with already loaded extra libraries
        std::map<std::string, void*> libMap;

        // aux function to load libraries that generated code might need
        // called by Load before loading the main library
        void LoadDependencies(std::string dirName);

    public:
        /** Need the path to the source tree so that we can load the correct code.
          Need to know where to send the generated message (coordinator)
         **/
        CodeLoaderImp(const char* _srcDir, EventProcessor& _coordinator);
        virtual ~CodeLoaderImp() {}

        // handler for the new message processing request
        MESSAGE_HANDLER_DECLARATION(LoadNewCode);
};

#endif // _CODELOADERIMP_H
