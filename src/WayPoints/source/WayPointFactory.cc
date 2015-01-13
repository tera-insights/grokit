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
/* The sole purpose of this this file is to allow all types of
     waypoints to be built.

     When a new types of waypoint is added, it has to be mentined here
     for the system to know how to construct it.

     Two tasks need to be accomplished
     1. add the include file
     2. add the constructor to the switch statement

*/

#include "JoinWayPointImp.h"
//#include "JoinMergerWayPointImp.h"
#include "AggWayPointImp.h"
#include "PrintWayPointImp.h"
#include "SelectionWayPointImp.h"
#include "TableScanWayPointImp.h"
#include "TableWayPointImp.h"
#include "TextLoaderWayPointImp.h"
#include "HashTableCleanerWayPointImp.h"
#include "WriterWayPointImp.h"
#include "TileJoinWayPointImp.h"
#include "GLAWayPointImp.h"
#include "GTWayPointImp.h"
#include "GISTWayPointImp.h"
#include "GIWayPointImp.h"
#include "CacheWayPointImp.h"
#include "ClusterWayPointImp.h"

using namespace std;

/* The only function. Takes a configuration objec as input
     and creates the waypoint of the correct type.

     NOTE: the configuration object is only looked at, it will not be
     harmed (not swapped).

   NOTE2: this function does not and should not actually configure the
   waypoint. It simply creates it.
*/

WayPointImp* WayPointFactory(const WayPointConfigureData &configData){

    switch (configData.Type()) {

        case JoinConfigureData::type :
             return new JoinWayPointImp;

//        case JoinMergeConfigureData::type :
//             return new JoinMergerWayPointImp;

        case SelectionConfigureData::type :
            return new SelectionWayPointImp;

        case PrintConfigureData::type :
            return new PrintWayPointImp;

        case AggregateConfigureData::type :
            return new AggWayPointImp;

        case TableConfigureData::type :
            return new TableWayPointImp;

        case TableScanConfigureData::type :
            return new TableScanWayPointImp;

        case TextLoaderConfigureData::type :
            return new TextLoaderWayPointImp;

        case WriterConfigureData::type :
            return new WriterWayPointImp;

        case HashTableCleanerConfigureData::type :
            return new HashTableCleanerWayPointImp;

        case TileJoinConfigureData::type :
            return new TileJoinWayPointImp;

        case GLAConfigureData::type :
            return new GLAWayPointImp;

        case GTConfigureData::type :
            return new GTWayPointImp;

        case GISTConfigureData::type :
            return new GISTWayPointImp;

        case GIConfigureData::type :
            return new GIWayPointImp;

        case CacheChunkConfigureData::type :
            return new CacheWayPointImp;
        case ClusterConfigureData::type:
            return new ClusterWayPointImp;

        default:
            FATAL ("Got some strange type of waypoint configuration. \n");
    }
}
