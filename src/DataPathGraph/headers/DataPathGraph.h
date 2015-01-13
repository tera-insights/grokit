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

#ifndef DPATH_GRAPH_H
#define DPATH_GRAPH_H

#include <string>
#include <stdexcept>

#include "EfficientMap.cc"
#include "InefficientMap.cc"
#include "ID.h"
#include "Swap.h"
#include "SerializeJson.h"

class DataPathGraph {

    private:

        typedef EfficientMap <WayPointID, QueryExitContainer> Links;

        // prints a set of links
        static void Print (Links &links) {

            int first = 1;
            for (links.MoveToStart (); !links.AtEnd (); links.Advance ()) {
                std::cout << "\n\t## ";
                links.CurrentKey ().Print ();
                std::cout << ": ";
                for (links.CurrentData ().MoveToStart (); links.CurrentData ().RightLength ();
                        links.CurrentData ().Advance ()) {
                    links.CurrentData ().Current ().Print ();
                    std::cout << "; ";
                }
            }
        }

        // this is all of the info we have about the links from/to a node in the graph
        struct NodeInfo {

            // this stuff is pretty self-explanatory
            Links linksTo;
            Links linksFrom;

            // usual constructor, destructor, swap
            NodeInfo () {}
            ~NodeInfo () {}
            void swap (NodeInfo &withMe) {
                linksTo.swap(withMe.linksTo);
                linksFrom.swap(withMe.linksFrom);
            }

            void Print () {
                std::cout << "linksTo: ";
                DataPathGraph :: Print (linksTo);
                std::cout << "\nlinksFrom: ";
                DataPathGraph :: Print (linksFrom);
                std::cout << "\n";
            }

            void toJson( Json::Value & dest ) const {
                dest = Json::Value(Json::objectValue);
                ToJson(linksTo, dest["linksTo"]);
                ToJson(linksFrom, dest["linksFrom"]);
            }

            void fromJson( const Json::Value & src ) {
                if( !src.isMember("linksTo") || !src.isMember("linksFrom") )
                    throw new std::invalid_argument("Attempting to deserialize DataPathGraph::NodeInfo from invalid JSON");

                FromJson(src["linksTo"], linksTo);
                FromJson(src["linksFrom"], linksFrom);
            }
        };

        friend void swap( NodeInfo& a, NodeInfo& b );
        friend void ToJson( const NodeInfo & src, Json::Value & dest );
        friend void FromJson( const Json::Value & src, NodeInfo & dest );

    private:
        // for each WayPointID, this will give us fast access to all of the in and out links
        typedef EfficientMap <WayPointID, NodeInfo> Graph;

        // the actual graph
        Graph myGraph;

    public:

        void Print () {

            for (myGraph.MoveToStart (); !myGraph.AtEnd (); myGraph.Advance ()) {
                myGraph.CurrentKey ().Print ();
                std::cout << "\n";
                myGraph.CurrentData ().Print ();
            }
        }

        void PrintDOT(std::ostream& out){

            // preamble
            out <<  "\n\t/* Graph */\n";
            out <<  "digraph G {\n\tcompound=true;\n\trankstep=1.25;\n";
            //out << "\tlabel=\"Data Path Network\";\n\tnode[shape=plaintext,fontsize=14, fontcolor=black];\n";
            out << "\tlabel=\"Data Path Network\";\n\tnode[shape=ellipse,fontsize=12,fontcolor=black,color=grey];\n";
            out << "\tbgcolor=white;\n\tedge [arrowsize=1,fontsize=10,color=blue,fontcolor=blue];\n";

            // nodes
            out << "\n\t/* Nodes */\n";
            {
                for (myGraph.MoveToStart (); !myGraph.AtEnd (); myGraph.Advance ()) {
                    std::string wName = myGraph.CurrentKey ().getName();
                    out << "\tsubgraph cluster_" << wName << " {label=\""
                        << wName << "\"; labelloc=\"b\";};\n";
                }
            }
            // edges
            out << "\n\t/* Relationships */\n";
            // terminating edge
            for (myGraph.MoveToStart (); !myGraph.AtEnd (); myGraph.Advance ()) {
                Links& links = myGraph.CurrentData().linksTo;
                std::string from = myGraph.CurrentKey ().getName();
                for (links.MoveToStart (); !links.AtEnd (); links.Advance ()) {
                    std::string to = links.CurrentKey ().getName();
                    // All query exits associated with this edge
                    std::string QEs;
                    for (links.CurrentData ().MoveToStart (); links.CurrentData ().RightLength ();
                            links.CurrentData ().Advance ()) {
                        QEs += links.CurrentData ().Current ().GetStr();
                    }
                    if (QEs == "")
                        QEs = "error";
                    out <<  "\tedge [label=\"" << QEs << "\"]" << "\t "
                        << from << "->" << to << ";\n";
                }
            }

            // finish
            out << "\n}\n";

        }

        // swaps two data path graphs
        void swap (DataPathGraph &withMe);

        // this adds a node to the graph... the second arg is all of the query-exit pairs
        // that end at that particular node
        void AddNode (WayPointID &myID);

        // this adds a link to the graph... the link goes from the first node to the
        // second... the given query-exit pair flows over that link
        void AddLink (WayPointID &from, WayPointID &to, QueryExit &pair);

        // this finds all of the waypoints that we need to traverse to send a hopping message or
        // data object to everything in "dests", given that our current location is "currentPos".
        // The routine produces "allSubsets", which has a bunch of (waypoint, QueryExitContainer)
        // pairs.  The first item in each pair is the ID of a waypoint that we need to hop to, and
        // the second is the set of all of the QueryExits that will be sent along that link
        void FindAllRoutings (WayPointID &currentPos, QueryExitContainer &dests,
                InefficientMap <WayPointID, QueryExitContainer> &allSubsets);

        // this finds all of the waypoints that we need to traverse to in order to send
        // message upstream from "currentPos" to the waypoint/waypoints that produce data for
        // the QueryExit "dest".  The resulting set of waypoints is put into "nextOnes".
        void FindUpstreamWaypoints (WayPointID &currentPos, QueryExit &dest, WayPointIDContainer &nextOnes);

        void toJson( Json::Value & dest ) const {
            ToJson(myGraph, dest);
        }

        void fromJson( const Json::Value & src ) {
            FromJson( src, myGraph );
        }
};

// STL-compatible swap function
inline
void swap( DataPathGraph& a, DataPathGraph& b ) {
    a.swap(b);
}

inline
void swap( DataPathGraph::NodeInfo& a, DataPathGraph::NodeInfo& b ) {
    a.swap(b);
}

inline
void ToJson( const DataPathGraph::NodeInfo & src, Json::Value & dest ) {
    src.toJson(dest);
}

inline
void FromJson( const Json::Value & src, DataPathGraph::NodeInfo & dest ) {
    dest.fromJson(src);
}

inline
void ToJson( const DataPathGraph & src, Json::Value & dest ) {
    src.toJson(dest);
}

inline
void FromJson( const Json::Value & src, DataPathGraph & dest ) {
    dest.fromJson(src);
}

#endif
