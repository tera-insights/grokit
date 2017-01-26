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

#include <sstream>
#include <string>
#include <cmath>
#include <vector>

#include "PrintWayPointImp.h"
#include "CPUWorkerPool.h"
#include "EEExternMessages.h"
#include "EventProcessor.h"
#include "SerializeJson.h"
#include "Errors.h"

#include "WPFExitCodes.h"

using namespace std;

// Anonymous namespace for some static functions
namespace {
    void BeginCSV( FILE * file, PrintFileInfo & info ) {
        PrintHeader & header = info.get_header();
        std::string & separator = info.get_separator();

        int nRows = 0;

        // Figure out the maximum number of rows needed
        FOREACH_TWL(col, header) {
            nRows = std::max(nRows, col.Length());
        } END_FOREACH;

        // Vector of output streams to build each row
        vector<ostringstream> rows(nRows);

        bool firstCol = true;
        FOREACH_TWL(col, header) {
            int rowNum = 0;
            FOREACH_TWL(value, col) {
                if( !firstCol )
                    rows[rowNum] << separator;

                rows[rowNum] << value;

                rowNum++;
            } END_FOREACH;

            // Add blank values if this column doesn't have enough rows
            if( !firstCol ) {
                for(; rowNum < nRows; rowNum++) {
                    rows[rowNum] << separator;
                }
            }

            firstCol = false;
        } END_FOREACH;

        // Print out the finished rows
        for( auto & row : rows ) {
            fprintf(file, "%s\n", row.str().c_str());
        }
    }

    void EndCSV( PrintFileObj & info ) {
        // nothing
    }

    void BeginJSON( FILE * file, PrintFileInfo & info) {
        PrintHeader & header = info.get_header();

        Json::Value hVal;
        header.toJson(hVal);

        Json::FastWriter writer;
        std::string hStr = writer.write(hVal);
        hStr = hStr.substr(0, hStr.length()-1); // the writer adds a newline, remove it

        fprintf(file, "{ \"__type__\": \"table\", \"header\": %s, \"content\": [ ", hStr.c_str() );
    }

    void EndJSON( PrintFileObj & info ) {
        FILE * file = info.get_file();

        fprintf(file, "\n");
    }

    FILE * BeginFile( PrintFileInfo & info ) {
        string& fName = info.get_file();
        string& fType = info.get_type();

        FILE* str = fopen(fName.c_str(), "w");
        if ( str == nullptr ){
            fprintf(stderr, "File %s could not be opened in PRINT: %s",
                fName.c_str(), strerror(errno));
            return nullptr;
        }

        if( fType == "csv" ) {
            BeginCSV( str, info );
        }
        else if( fType == "json" ) {
            BeginJSON( str, info );
        }
        else {
            FATAL("File %s has unknown type %s in Print", fName.c_str(), fType.c_str());
        }

        return str;
    }

    void EndFile( PrintFileObj & info ) {
        string& fType = info.get_type();
        FILE * file = info.get_file();
        if( fType == "csv" ) {
            EndCSV( info );
        } else if( fType == "json" ) {
            EndJSON( info );
        } else {
            FATAL("File has unknown type %s in Print", fType.c_str());
        }

        fclose(file);
    }
} // end anonymous namespace

extern EventProcessor globalCoordinator; /* who to contact when we
                                            fihish some queries */

PrintWayPointImp :: PrintWayPointImp () {PDEBUG ("PrintWayPointImp :: PrintWayPointImp ()");}
PrintWayPointImp :: ~PrintWayPointImp () {PDEBUG ("PrintWayPointImp :: PrintWayPointImp ()");}

void PrintWayPointImp :: TypeSpecificConfigure (WayPointConfigureData &configData) {
    PDEBUG ("PrintWayPointImp :: TypeSpecificConfigure ()");

    // load up the list where we will place done queries
    PrintConfigureData tempConfig;
    tempConfig.swap (configData);

    // tell people that we are ready to go with our queries... these messages will
    // eventually make it down to the table scan, which will begin producing data
    QueryExitContainer endingOnes;
    GetEndingQueryExits (endingOnes);
    QueryToFileInfoMap queriesInfo;
    queriesInfo.swap(tempConfig.get_queriesInfo());

    for (endingOnes.MoveToStart (); endingOnes.RightLength (); endingOnes.Advance ()) {

        // get the meta data
        QueryExit tempExit = endingOnes.Current (), tempExitCopy = endingOnes.Current ();
#ifdef DEBUG
        cout << "Print: Asking for ";
        tempExit.Print ();
        cout << " to start.\n";
#endif  // DEBUG

        // Open the file and write the header for the query
        PrintFileInfo& info = queriesInfo.Find(tempExit.query);
        std::string fSeparator = info.get_separator();
        std::string fType = info.get_type();
        FILE * str = BeginFile(info);
        // create the counter with the limit set correctly
        int64_t limit = info.get_limit() == -1 ? 1ULL << 60 /* really big limit*/: info.get_limit() ;
        SwapifiedDCptr counter = new DistributedCounter(limit);
        QueryID query = tempExit.query;
        counters.Insert(query, counter);

        // TODO: Eject query if nullptr
        FATALIF( str == nullptr, "Print was unable to open file for writing, aborting" );

        PrintFileObj pfo(str, fSeparator, fType);
        query=tempExit.query;
        streams.Insert(query, pfo);

        WayPointID myID = GetID (), myIDCopy = GetID ();

        // create the actual notification fiest
        StartProducingMsg startProducing (myID, tempExit);

        // now wrap it up in a hopping upstream message
        HoppingUpstreamMsg myMessage (myIDCopy, tempExitCopy, startProducing);
        SendHoppingUpstreamMsg (myMessage);
    }
}

void PrintWayPointImp :: RequestGranted( GenericWorkToken & token ) {
    PDEBUG ("PrintWayPointImp :: RequestGranted ()");

    if( queriesToFinalize.IsEmpty() ) {
        GiveBackToken(token);
        return;
    }

    CPUWorkToken myToken;
    myToken.swap (token);

    QueryExitContainer whichOnes;
    whichOnes.swap(queriesToFinalize);

    QueryExitContainer whichOnesCopy;
    whichOnesCopy.copy(whichOnes);

    QueryToFileMap streamsOut;

    FOREACH_TWL(qe, whichOnes) {
        QueryID query = qe.query;
        PrintFileObj file;
        file.copy(streams.Find(query));
        streamsOut.Insert(query, file);
    } END_FOREACH;

    PrintFinalizeWorkDescription workDesc( whichOnes, streamsOut );

    HistoryList lineage;
    WayPointID myID = GetID();
    WorkFunc myFunc = GetWorkFunction(PrintFinalizeWorkFunc::type);
    myCPUWorkers.DoSomeWork(myID, lineage, whichOnesCopy, myToken, workDesc, myFunc);
}

void PrintWayPointImp :: DoneProducing (QueryExitContainer &whichOnes, HistoryList &history,
        int result, ExecEngineData& data) {
    PDEBUG ("PrintWayPointImp :: DoneProducing()");

    QueryExitContainer whichOnesC;
    QueryExitContainer endingOnes;

    switch( result ) {
    case WP_PROCESS_CHUNK:
        // send an ack message back down through the graph to let them know we are done
        SendAckMsg (whichOnes, history);
        break;
    case WP_FINALIZE:
        whichOnesC.copy(whichOnes);

        FOREACH_TWL(qe, whichOnes ) {
            if( qe.exit == GetID() ) {
                QueryExit tmp = qe;
                endingOnes.Append(tmp);

                QueryID tmp2 = qe.query;
                QueryID dummy;
                PrintFileObj fileObj;
                streams.Remove(tmp2, dummy, fileObj);;

                // Close the file
                EndFile(fileObj);
            }
        } END_FOREACH;

        // send a message to the coordinator that we are done
        QueriesDoneMessage_Factory(globalCoordinator, whichOnesC);
        break;
    }
}

void PrintWayPointImp :: ProcessHoppingDownstreamMsg (HoppingDownstreamMsg &message) {
    PDEBUG ("PrintWayPointImp :: ProcessHoppingDownstreamMsg()");

    // see if we have a query done message
    if (CHECK_DATA_TYPE (message.get_msg (), QueryDoneMsg)) {

        // do the cast via a swap
        QueryDoneMsg temp;
        temp.swap (message.get_msg ());

        QueryExitContainer whichOnesC;
        whichOnesC.copy(temp.get_whichOnes());

        queriesToFinalize.SuckUp(whichOnesC);

        // Get a work token so we can finalize the files
        RequestTokenNowDelayOK(CPUWorkToken::type);

    } else {
        FATAL ("Why did I get some hopping downstream message that was not a query done message?\n");
    }

}

void PrintWayPointImp :: ProcessHoppingDataMsg (HoppingDataMsg &data) {
    PDEBUG ("PrintWayPointImp :: ProcessHoppingDataMsg()");

    // request a work token to actually run the print
    GenericWorkToken returnVal;
    if (!RequestTokenImmediate (CPUWorkToken::type, returnVal)) {

        // if we do not get one, then we will just return a drop message to the sender
        SendDropMsg (data.get_dest (), data.get_lineage ());
        return;
    }

    // convert the token into the correct type
    CPUWorkToken myToken;
    myToken.swap (returnVal);

    // extract the chunk we need to print
    ChunkContainer temp;
    data.get_data ().swap (temp);

    // fileDescriptors of all queries involved
    QueryToFileMap streamsOut;

    // Counters so we limit the output of print
    QueryToCounters countersOut;

    // create the work spec that we will actuall have executed
    QueryExitContainer whichOnes;
    whichOnes.copy (data.get_dest ());
    FOREACH_TWL(el, whichOnes){
        QueryID query=el.query;
        PrintFileObj file;
        file.copy(streams.Find(query));
        streamsOut.Insert(query, file);

        query = el.query;
        SwapifiedDCptr counter;
        counter.copy(counters.Find(query));
        countersOut.Insert(query, counter);
    }END_FOREACH;

    PrintWorkDescription workDesc (whichOnes, streamsOut, temp.get_myChunk (), countersOut);

    // now actually get the work taken care of!
    WayPointID myID;
    myID = GetID ();
    WorkFunc myFunc = GetWorkFunction (PrintWorkFunc::type);
    myCPUWorkers.DoSomeWork (myID, data.get_lineage (), data.get_dest (), myToken, workDesc, myFunc);
}

