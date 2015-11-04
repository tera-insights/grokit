// Copyright 2013 Tera Insights, LLC. All Rights Reserved.

#ifndef CACHE_WAY_POINT_IMP
#define CACHE_WAY_POINT_IMP

#include <cinttypes>

#include "ID.h"
#include "History.h"
#include "Tokens.h"
#include "Constants.h"
#include "WayPointImp.h"
#include "ExecEngineData.h"
#include "EfficientMap.h"
#include "Swapify.h"

class CacheWayPointImp : public WayPointImp {
private:
    typedef EfficientMap<Keyify<uint64_t>, CachedChunk> ChunkMap;

    bool allReceived;

    ChunkMap chunksAvailable;
    ChunkMap chunksOut;
    ChunkMap chunksDone;

    uint64_t nextID;

    uint64_t nChunks;
    uint64_t nAcked;

public:
    CacheWayPointImp();
    ~CacheWayPointImp();

    void TypeSpecificConfigure( WayPointConfigureData & configData );
    void RequestGranted (GenericWorkToken &returnVal);
    void ProcessHoppingUpstreamMsg (HoppingUpstreamMsg &message);
    void ProcessHoppingDataMsg (HoppingDataMsg &data);
    void ProcessHoppingDownstreamMsg (HoppingDownstreamMsg &message);
    void ProcessDropMsg (QueryExitContainer &whichOnes, HistoryList &lineage);
    void ProcessAckMsg (QueryExitContainer &whichOnes, HistoryList &lineage);
    void DoneProducing (QueryExitContainer &whichOnes, HistoryList &history,
        int returnVal, ExecEngineData& data);
};

#endif // CACHE_WAY_POINT_IMP
