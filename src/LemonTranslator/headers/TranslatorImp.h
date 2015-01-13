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
#ifndef __TRANSLATOR_IMP_H_
#define __TRANSLATOR_IMP_H_

#include "ID.h"
#include "EventProcessorImp.h"
#include "MessageMacros.h"
#include "LemonTranslator.h"
#include <antlr3.h>

/* Translator object that gets instructions in the high level language
   and comes up with the instructions for the entire system. The
   translator will do the following things:

   1. Figure out how to update the DataPathGraph
   2. Generate code for all waypoints that changed
   3. Generate instructions for file scanners for all new queries
   4. Compile the code and make it available in a shared directory

   A single message is sent to the Coordinator with all the info when
   queries change. A separate message is send for each "run"
   command. If queries are to be executed simultaneously, make sure
   they are all listed under a single "run".

NOTE: the translator does not talk directly to the file scanners
and has a WayPointID associated with them instead of a
TableScanID. The conversion has to be done by the Coordinator or
Executin engine.

NEED: add "dot" plotting capabilities to LemonGraph

NOTE: Should the Attribute manager be a part of the translator
instead of being global?

*/

class TranslatorImp : public EventProcessorImp {
    private:
        EventProcessor coordinator; // always the same, send messages back here

        LemonTranslator lTrans; // the lemon translator. does most of the work

        bool ParsePiggy(    pANTLR3_INPUT_STREAM    input);

    public:
        // constructor. We do not do much, except set the coordinator
        TranslatorImp(EventProcessor& _coordinator, bool batchMode);

        // destructor doing notning
        virtual ~TranslatorImp(void){ }

        // MESSAGE PROCESSING METHODS
        // processor for the translator
        MESSAGE_HANDLER_DECLARATION(Translate);
        // processor for finished queries
        MESSAGE_HANDLER_DECLARATION(DeleteQueries);

        // parser ; return false if failed parsing
        bool ParseFile(const char* filename);

};


#endif // __TRANSLATOR_IMP_H_
