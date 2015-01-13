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

#ifndef WRITER_H
#define WRITER_H

#include "WayPointImp.h"

// this class is super-simple, since selection can use the default implementation of almost everything
class WriterWayPointImp : public WayPointImp {

    public:

        // const and destr
        WriterWayPointImp ();
        virtual ~WriterWayPointImp ();

        // methods this guy over-rides
        void ProcessHoppingDataMsg (HoppingDataMsg &data);
        void DoneProducing (QueryExitContainer &whichOnes, HistoryList &history, int result, ExecEngineData& data);
        void ProcessHoppingDownstreamMsg (HoppingDownstreamMsg &message);
};

#endif
