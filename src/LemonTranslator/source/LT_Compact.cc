#include "LT_Compact.h"

using namespace std;

bool LT_Compact :: AddCompact( QueryID query ) {
    queriesCovered.Union(query);
    return true;
}

bool LT_Compact :: PropagateDown( QueryID query, const SlotSet& atts, SlotSet& rez, QueryExit qe ) {
    used[qe.query] = atts;
    rez = atts;

    queryExit.Insert(qe);

    return true;
}

bool LT_Compact ::PropagateUp(QueryToSlotSet& result) {
    result.clear();
    CheckQueryAndUpdate(downAttributes, result);
    downAttributes.clear();

    return true;
}

bool LT_Compact ::GetConfig(WayPointConfigureData& where) {
    WayPointID wpID = GetId();

    WorkFunc tempFunc = NULL;
    WorkFuncContainer myWorkFuncs;
    CompactProcessChunkWorkFunc func(tempFunc);
    myWorkFuncs.Insert(func);

    QueryExitContainer endingQEs;
    QueryExitContainer thruQEs;
    GetQueryExits(thruQEs, endingQEs);

    CompactChunkConfigureData config(wpID, myWorkFuncs, endingQEs, thruQEs);

    config.swap(where);

    return true;
}

Json::Value LT_Compact ::GetJson() {
    Json::Value out(Json::objectValue);

    IDInfo info;
    GetId().getInfo(info);

    out[J_NAME] = info.getName();
    out[J_TYPE] = JN_COMPACT_WP;

    SlotToQuerySet reverse;
    AttributesToQuerySet(used, reverse);
    out[J_ATT_MAP] = JsonAttToQuerySets(reverse);

    return out;
}
