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
#include "TwoWayList.h"
#include "EfficientMap.h"
#include "TextLoaderWayPointImp.h"
#include "CPUWorkerPool.h"
#include "Stl.h"
#include "Logging.h"
#include "DictionaryManager.h"
#include "Profiling.h"

// Temporary hack
#include "Timer.h"

using namespace std;

const double TextLoaderWayPointImp::CACHE_INTERVAL = 0.2;

TextLoaderWayPointImp :: TextLoaderWayPointImp () {
    PDEBUG ("TextLoaderWayPointImp :: TextLoaderWayPointImp ()");
    numStreams = 0;
    tokensRequested = 0 ;
    chunksOut = 0;
    requestCnt = 0;

    // Temporary hack
    lastCacheSend = 0.0;
}

TextLoaderWayPointImp :: ~TextLoaderWayPointImp () {
    PDEBUG ("~TextLoaderWayPointImp :: TextLoaderWayPointImp ()");
    // nothing for now
}

void TextLoaderWayPointImp :: RequestTokens(){
    PDEBUG ("TextLoaderWayPointImp :: RequestTokens ()");
    // the goal is to have around either enough tokens to process all
    // streams or ar many as to keep total number of chunks
    // build+outForWritting at 2*numStreams

    // requests to keep all loaders bussy
    int noReq = 0;
    int dblBuf = 0;
    if( chunkCache.Length() == 0 ) {
        noReq = tasks.Length() - tokensRequested;
        dblBuf = 2*numStreams - chunksOut - tokensRequested;
    }
    else {
        noReq = chunkCache.Length() - tokensRequested;
        noReq = noReq < 0 ? 0 : noReq;
    }

    FATALIF (noReq < 0, "This should not be smaller than 0");

    // is that too many?
    //WARNINGIF(noReq > dblBuf, "Too many request: %d\n", noReq);

    // queue up some more work requests
    // one for each element of the list
    for (int i=0; i<noReq; i++) {
        tokensRequested++;
        RequestTokenNowDelayOK (CPUWorkToken::type);
    }
}

void TextLoaderWayPointImp :: SendCachedChunk( CachedChunk& chunk ) {
    QueryExitContainer &whichOnes = chunk.get_whichExits();
    HistoryList &lineage = chunk.get_lineage();
    ChunkContainer &chunkCont = chunk.get_myChunk();

    // Send the message
    SendHoppingDataMsg( whichOnes, lineage, chunkCont );
}

void TextLoaderWayPointImp :: TypeSpecificConfigure (WayPointConfigureData &configData) {
    PDEBUG ("TextLoaderWayPointImp :: TypeSpecificConfigure ()");

    // first, extract the extra config info
    TextLoaderConfigureData tempConfig;
    tempConfig.swap (configData);

    // store query exits
    myExits.swap(tempConfig.get_queries());

    // invent a tID for ourselves
    IDInfo info;
    GetID().getInfo(info);
    name = info.getName();
    TableScanID newID(name);
    tID.swap(newID);

    // for each string in the input, create a task

    FOREACH_STL(file, tempConfig.get_files()){
        FILE* stream = fopen(file.c_str(), "r");
        if ( stream==NULL){
            printf( "File %s could not be opened in TextLoader:", file.c_str());
            perror(NULL);
            break;
        }

        // set the buffer at 1M so that we read faster
        // default buffer size is really bad
        setvbuf(stream, NULL /* system allocated */, _IOFBF, 1<<20 /* 1MB */);

        LOG_ENTRY_P(1, "Loader %s Started", file.c_str());

        TextLoaderDS task(stream, file);
        tasks.Append(task);
        numStreams++;

    }END_FOREACH

}

void TextLoaderWayPointImp :: RequestGranted (GenericWorkToken &returnVal) {
    PDEBUG ("TextLoaderWayPointImp :: RequestGranted ()");

    CPUWorkToken myToken;
    myToken.swap (returnVal);

    // the token is not out no matter what
    tokensRequested--;

    // If we have chunks cached, serve those first and return the token.
    if( chunkCache.Length() > 0 ) {
        double curTime = global_clock.GetTime();

        if( curTime > lastCacheSend + CACHE_INTERVAL ) {
            CachedChunk myChunk;
            chunkCache.MoveToStart();
            chunkCache.Remove( myChunk );

            // Send cached chunk
            SendCachedChunk( myChunk );
            lastCacheSend = curTime;
        }

        GiveBackToken( myToken );

        return;
    }

    // do we even have some work we could do?
    // this shoudl probably not happen but better safe than sorry
    FATALIF(tasks.Length() == 0, "This Should never happen");

    // get a task out
    TextLoaderDS task;
    // remove from the beginning so that we behave like a FIFO
    tasks.MoveToStart();
    tasks.Remove(task);

    // set up the lineage... since the chunk originates here, we create the object from scratch
    QueryExitContainer myOutputExits;
    myOutputExits.copy (myExits);
    QueryExitContainer myOutputExitsCopy;
    myOutputExitsCopy.copy (myExits);


    ChunkID cID(requestCnt, tID);
    requestCnt++; // next request in future
    TextLoaderHistory myHistory (GetID (), cID, task.get_file());
    HistoryList tempList;
    tempList.Insert (myHistory);

    // set up the work description
    TextLoaderWorkDescription workDesc (task.get_stream(), task.get_file(),
            myOutputExits);

    // now, actually get the chunk sent out!  Again, note that here we use a CPU to do this...
    // but that is just because we have a toy table scan imp
    WorkFunc myFunc = GetWorkFunction (TextLoaderWorkFunc::type);
    WayPointID temp1 = GetID ();
    myCPUWorkers.DoSomeWork (temp1, tempList, myOutputExitsCopy, myToken, workDesc, myFunc);

    // ask for more
    RequestTokens();
}

void TextLoaderWayPointImp :: ProcessHoppingUpstreamMsg (HoppingUpstreamMsg &message) {
    PDEBUG ("TextLoaderWayPointImp :: ProcessHoppingUpstreamMsg ()");

    FATALIF (!CHECK_DATA_TYPE (message.get_msg (), StartProducingMsg),
            "Strange, why did a text loader get a HUS of a type that was not 'Start Producing'?");

    // access the content of the message
    StartProducingMsg myMessage;
    message.get_msg ().swap (myMessage);

    // see what query we are asked to start
    QueryExit &queryToStart = myMessage.get_whichOne ();
    cout << "About to start ";
    queryToStart.Print ();
    cout << "\n";

    RequestTokens();
}

void TextLoaderWayPointImp :: ProcessDropMsg (QueryExitContainer &whichExits, HistoryList &lineage) {

    PDEBUG ("TextLoaderWayPointImp :: ProcessDropMsg ()");

    PROFILING2_INSTANT("chn", 1, GetName());

    // Get chunk from mapping
    EXTRACT_HISTORY_ONLY( lineage, myHistory, TextLoaderHistory );
    ChunkID cID = myHistory.get_whichChunk();
    myHistory.swap(lineage.Current());

    ChunkContainer chunkCont;
    FATALIF( ! chunkMap.IsThere( cID ), "Got drop for chunk I don't know about!" );
    chunkCont.copy( chunkMap.Find( cID ) );

    CachedChunk cCache(chunkCont, lineage, whichExits);
    chunkCache.Append(cCache);

    RequestTokens();
}

void TextLoaderWayPointImp :: DoneProducing (QueryExitContainer &whichOnes, HistoryList &history,
        int result, ExecEngineData& data) {
    PDEBUG ("TextLoaderWayPointImp :: DoneProducing ()");
    TextLoaderHistory myHistory;
    history.MoveToStart();
    myHistory.swap(history.Current());
    string file = myHistory.get_file();
    ChunkID cID = myHistory.get_whichChunk();
    // put it back
    myHistory.swap(history.Current());

    TextLoaderResult tempResult;
    tempResult.swap(data);

    // form the chunk that goes out
    ChunkContainer chkCont(tempResult.get_myChunk());
    ChunkContainer contCopy;
    contCopy.copy(chkCont);
    // put it into data (what should have been)
    data.swap(chkCont);
    chunksOut++; // one chunk out

    // Add chunk to mapping
    chunkMap.Insert( cID, contCopy );

    // if the result is 1, the stream exhausted the input
    if (result == 1) {
        numStreams--; // one stream done

        // close the stream
        fclose(tempResult.get_stream());

        LOG_ENTRY_P(2, "Text Loader %s finished processing file %s.",
                name.c_str(), file.c_str()) ;

    } else {
        // just take the returned stream back to the tasks list
        TextLoaderDS task(tempResult.get_stream(), file);
        tasks.Append(task);

    }

    PROFILING2_INSTANT("chp", 1, GetName());

    RequestTokens();

}

void TextLoaderWayPointImp :: ProcessAckMsg (QueryExitContainer &whichExits, HistoryList &lineage) {
    PDEBUG ("TextLoaderWayPointImp :: ProcessAckMsg ()");

    // make sure that the HistoryList has one item that is of the right type
    EXTRACT_HISTORY_ONLY(lineage, myHistory, TextLoaderHistory);

    PROFILING2_INSTANT("cha", 1, GetName());

    chunksOut--; // one chunk less flying

    // Remove chunk from mapping
    ChunkID cID = myHistory.get_whichChunk();
    ChunkID key;
    ChunkContainer value;
    FATALIF( ! chunkMap.IsThere( cID ), "Got an ACK for a chunk I don't know about!" );
    chunkMap.Remove( cID, key, value );

    // are we done with everything?
    if (chunksOut == 0 && numStreams == 0) {
        QueryExitContainer allCompleteCopy;
        allCompleteCopy.copy (myExits);
        QueryDoneMsg qDone (GetID (), myExits); // do not need my exits anymore
        HoppingDownstreamMsg myOutMsg (GetID (), allCompleteCopy, qDone);
        SendHoppingDownstreamMsg (myOutMsg);

        LOG_ENTRY_P(2, "Text Loader %s finished processing ALL files.",
                name.c_str()) ;

        // make sure we merge the local dictionaries and flush
        DictionaryManager::Flush();
    }

    RequestTokens();
}


