#include "LT_Cache.h"

using namespace std;

bool LT_Cache :: AddCaching( QueryID query ) {
    queriesCovered.Union(query);
    return true;
}

bool LT_Cache :: PropagateDown( QueryID query, const SlotSet& atts, SlotSet& rez, QueryExit qe ) {
    used[qe.query] = atts;
    rez = atts;

    queryExit.Insert(qe);

    return true;
}

bool LT_Cache ::PropagateUp(QueryToSlotSet& result) {
    result.clear();
    CheckQueryAndUpdate(downAttributes, result);
    downAttributes.clear();

    return true;
}

bool LT_Cache ::GetConfig(WayPointConfigureData& where) {
    WayPointID wpID = GetId();

    WorkFunc tempFunc = NULL;
    WorkFuncContainer myWorkFuncs;
    CacheChunkWorkFunc func(tempFunc);
    myWorkFuncs.Insert(func);

    QueryExitContainer endingQEs;
    QueryExitContainer thruQEs;
    GetQueryExits(thruQEs, endingQEs);

    CacheChunkConfigureData config(wpID, myWorkFuncs, endingQEs, thruQEs);

    config.swap(where);

    return true;
}

Json::Value LT_Cache ::GetJson() {
    Json::Value out(Json::objectValue);

    IDInfo info;
    GetId().getInfo(info);

    out[J_NAME] = info.getName();
    out[J_TYPE] = JN_CACHE_WP;

    return out;
}
