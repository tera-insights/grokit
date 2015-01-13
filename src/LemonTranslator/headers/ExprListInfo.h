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
#ifndef _EXPR_LIST_INFO_
#define _EXPR_LIST_INFO_

#include <sstream>
#include "ArgFormat.h"
#include "json.h"

#ifdef DTM_DEBUG
#include <iostream>
#endif

/* Class to provide an interface for parsing expressions

*/

class ExprListInfo {
    static int cVarCNT; // counter for constants
    // used to generate constants of the form ct3

    struct Element {
        std::string sExpr; // the expression
        std::string type; // the normalized type
        bool isCT; // is the expression constant?
        Json::Value json; // JSON representation of expression

        Element(std::string _sExpr, std::string _type, bool _isCT, const Json::Value & _json):
            sExpr(_sExpr), type(_type), isCT(_isCT), json(_json){}
    };

    std::vector<Element> list;
    bool isAllCT; // are all expressions constant?
    bool isAnyCT; // is any of the expr constant?

public:
    ExprListInfo(){ isAllCT=true; }

    // get the next constant number
    static int NextVar(void){ return cVarCNT++; }

    void Add(std::string sExpr, std::string type, bool isCT, const Json::Value & json){
        list.push_back(Element(sExpr, type, isCT, json));
        isAllCT = isAllCT & isCT;
        isAnyCT = isAnyCT | isCT;
    }

    bool IsListConstant(void){ return isAllCT; }
    bool IsAnyConstant(void){ return isAnyCT; }

    // Return number of expressions collected
    size_t size(void) const {
        return list.size();
    }

    // Return the JSON of an expression
    Json::Value getJson( const size_t index ) const {
        return list[index].json;
    }

    // Return a JSON representation of the list of expressions
    Json::Value getJsonList( void ) const {
        Json::Value list(Json::arrayValue);
        for( Element elem : this->list ) {
            list.append(elem.json);
        }
        return list;
    }

    // returns a ',' separated std::list of types
    std::string GetTypesDesc(void){
        std::string rez;

        if( list.size() > 0 )
            rez += list[0].type;

        for( int i = 1; i < list.size(); ++i ) {
            rez += (", " + list[i].type);
        }

        return rez;
    }

    std::vector<std::string> GetListTypes(void){
        std::vector<std::string> listTypes;
        for( size_t i = 0; i < list.size(); ++i ) {
            listTypes.push_back(list[i].type);
        }
        return listTypes;
    }

    // function to print an expression in a std::vector of strings
    // each element coresponds to the printout for each expression
    // Note that constants are not extracted in this method. Prepare() must
    // be called beforehand for constant extraction, as this relies on
    // information from the data type manager.
    std::vector<std::string> Generate( ){
        std::vector<std::string> rez;
        for (int i=0; i<list.size(); i++){
            std::string lRez;

            lRez = list[i].sExpr;

            rez.push_back(lRez);
        }

        return rez;
    }

    /**
      Prepares the expression for printout generation.
      This method modifies the types and expressions of each element as
      necessary, using the information about the actual types and necessary
      conversions of the arguments given by the data type manager.

      During this process, constants will be pulled out. Being overly-greedy
      about pulling out constants shouldn't be too much of an issue,
      the compiler should get rid of any unnecessary variables.
      */
    void Prepare( std::string& cstStr, std::vector<ArgFormat> args = std::vector<ArgFormat>() ) {
        for( size_t i = 0; i < list.size(); ++i ) {

            if( i < args.size() ) {
                list[i].type = args[i].getType();
                list[i].sExpr = args[i].format(list[i].sExpr);
            }

            // This argument is constant. Create a new variable declaration in cstStr,
            // and replace the argument expression with the name of the new constant.
            if( list[i].isCT && ( i >= args.size() || !args[i].forceNoExtract() ) ) {
                std::ostringstream ctName;
                ctName << "ct" << NextVar();
                std::ostringstream gen;
                gen << "    ";
                gen << "const " << list[i].type << " " << ctName.str() << " = "
                    << list[i].sExpr << ";\n";

                cstStr += gen.str();
                list[i].sExpr = ctName.str();
            }
        }
    }
};

#endif // _EXPR_LIST_INFO_
