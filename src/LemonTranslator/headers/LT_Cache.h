// Copyright 2013 Tera insights, LLC. All Rights Reserved.

#ifndef _LT_CACHE_H_
#define _LT_CACHE_H_

#include "LT_Waypoint.h"

class LT_Cache : public LT_Waypoint {
    public:
        LT_Cache(WayPointID id): LT_Waypoint(id) { }

        virtual WaypointType GetType() { return CacheWaypoint; }

        virtual bool AddCaching( QueryID query );

        virtual bool PropagateDown( QueryID query, const SlotSet& atts, SlotSet& rez, QueryExit qe );
        virtual bool PropagateUp(QueryToSlotSet& result);

        virtual bool GetConfig(WayPointConfigureData& where);

        virtual Json::Value GetJson();
};

#endif // _LT_CACHE_H_
