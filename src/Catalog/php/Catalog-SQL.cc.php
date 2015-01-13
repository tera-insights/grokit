/** This is the SQL portion of the catalog code */

<?php
//
//  Copyright 2013 Tera Insights LLC
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
?>
<?php
require_once('SQLite.php');
?>



#include <list>

#include "Errors.h"
#include "MetadataDB.h"
#include "Catalog.h"
#include "CommunicationFramework.h"

#include <sstream>

#include "JsonAST.h"

using namespace std;

/**
     The information in the catalog is stored into the following relations:

     CatalogRelations:
        relation:TEXT,
        relID: INTEGER PRIMARY KEY,
        size: INTEGER

     CatalogAttributes:
        relID: int,
        attID: INT PRIMARY KEY, corresponds to colNo
        attribute:TEXT,
        type:TEXT,
        jType:TEXT,
        uniqueVals:INTEGER

     CatalogConstants:
        name: TEXT,
        value: TEXT
        type: TEXT  can only be: string, integer, double

     CatalogClusters:
        relID: INTEGER PRIMARY KEY, corresponds to CatalogRelations.relID
        attID: INTEGER corresponds to CatalogAttributes.attID,

 */

Catalog::Catalog() :
// Explicitly initialize all members, as allocations view `new` have no
// guarantee that the members will be default initialized if you don't specify
    tableAliases(),
    intConstants(),
    doubleConstants(),
    stringConstants(),
    constMutex(),
    diskPaths(),
    diskMutex(),
    schemas(),
    schemaMutex()
{


<?php
grokit\sql_open_database( 'GetMetadataDB() ' );
?>
;

    /** create the relations if necessary */

<?php
grokit\sql_statements_norez( <<<'EOT'
"
    CREATE TABLE IF NOT EXISTS CatalogRelations (
        /* relation is the relation name (same as relName in table Relations)*/
        /* relID is an id for the catalog only (not related with table Relations) */
        /* size contains statistics of the relation */
        relation				TEXT,
        relID					INTEGER				PRIMARY KEY,
        size					INTEGER
    );

    CREATE TABLE IF NOT EXISTS CatalogAttributes (
        /* physical layout column */
        /* type is the system type of the attribute */
        colNo					INTEGER,
        relID					INTEGER,
        type					TEXT,
        jType                   TEXT,
        attribute				TEXT,
        uniqueVals				INTEGER
    );

    CREATE TABLE IF NOT EXISTS CatalogConstants (
        name					TEXT,
        value					TEXT,
        type					TEXT
    );

  CREATE TABLE IF NOT EXISTS CatalogClusters (
    relID     INTEGER PRIMARY KEY,
    attID     INTEGER,
    FOREIGN KEY(relID) REFERENCES CatalogRelations(relID),
    FOREIGN KEY(attID) REFERENCES CatalogAttributes(colNo)
  );
"
EOT
, [ ] );
?>
;

  /** Read the constants in the system and insert them into the internal representation */

<?php
grokit\sql_statement_table( <<<'EOT'
"
    SELECT name, value, type
    FROM CatalogConstants;
"
EOT
, [ 'name' => 'text', 'value' => 'text', 'type' => 'text', ], [ ] );
?>
{

    // is it an integer?
    if (strcasecmp((char *)type, "integer") == 0) {
      int val = atoi(value);
      string valn = string(name);

      // add it to the map
      intConstants[valn] = val;
    }

    // is it a double?
    else if (strcasecmp((char *)type, "double") == 0) {
      double val = atof(value);
      string valn = string(name);

      // add it to the map
      doubleConstants[valn] = val;
    }

    // is it a string?
    else if (strcasecmp((char *)type, "string") == 0) {
      string val = string(value);
      string valn = string(name);

      // add it to the map
      stringConstants[valn] = val;
    }

    // undefined
    else {
      FATAL("unknown data type %s", type);
    }

  }<?php
grokit\sql_end_statement_table();
?>
;


  /** Read the schema information. Relations first then the attributes */
  // first we make a list of relation IDs (quirky interface to the
  // internal datastructures of the catalog, it wants all the info on
  // a relation in a single step)

  list<int> relIds;
<?php
grokit\sql_statement_table( <<<'EOT'
"
    SELECT relID
    FROM CatalogRelations;
"
EOT
, [ 'relID' => 'int', ], [ ] );
?>
{
    relIds.push_back(relID);
  }<?php
grokit\sql_end_statement_table();
?>
;

  // now go relation by relation
  for (list<int>::iterator it = relIds.begin(); it!=relIds.end(); it++){
    string relName;
    long int numTuples;

<?php
grokit\sql_statement_table( <<<'EOT'
"
    SELECT relation, size
    FROM CatalogRelations
    WHERE relID=%d;
"
EOT
, [ 'name' => 'text', '_numTuples' => 'int', ], [ '(*it)', ] );
?>
{
      relName = name;
      numTuples = _numTuples;
    }<?php
grokit\sql_end_statement_table();
?>
;

    // get the attribute info for this relation

    AttributeContainer curAtts;
    int attIndex=0;

    Attribute clusterAtt;
    int clusterAttColNo = -1;
<?  grokit\sql_statement_table(<<<'EOT'
"
  SELECT attID
  FROM CatalogClusters
  WHERE relID=%d;
"
EOT
, [ 
  'attID' => 'int'
], [ '(*it)', ]);
?>
    clusterAttColNo = attID;
<?  grokit\sql_end_statement_table(); ?>

<?php
grokit\sql_statement_table( <<<'EOT'
"
      SELECT attribute, uniqueVals, type, jType, colNo
      FROM CatalogAttributes
      WHERE relID=%d
            ORDER BY colNo;
"
EOT
, [ '_attName' => 'text', 'attUniques' => 'int', 'typec' => 'text', 'jTypec' => 'text', 'colNo' => 'int', ], [ '(*it)', ] );
?>
{

      string attName = string(_attName);
      string attType = typec;
      Json::Reader jReader;
      Json::Value attJType;
      jReader.parse(jTypec, attJType);

      Attribute att(attName, attType, attJType, attIndex++, attUniques);
      if( colNo == clusterAttColNo ) {
        att.CopyTo(clusterAtt);
      }

      curAtts.Append(att);

    }<?php
grokit\sql_end_statement_table();
?>
;


    // build the schema and add it to the catalog
    string nothing;
    Schema newSch(curAtts, relName, nothing, numTuples, clusterAtt);
    schemas.Insert(newSch);

  }
<?php
grokit\sql_close_database();
?>
;
}

void Catalog::SaveCatalog(){
  cout << "Saving the catalog " << endl;

<?php
grokit\sql_open_database( 'GetMetadataDB() ' );
?>
;
<?php
grokit\sql_statements_norez( <<<'EOT'
"
        DELETE FROM CatalogConstants;
        "
EOT
, [ ] );
?>
;

<?php
grokit\sql_statement_parametric_norez( <<<'EOT'
"
        INSERT INTO CatalogConstants(name, value, type) VALUES (?1, ?2, ?3);
        "
EOT
, [ 'text', 'text', 'text', ], [ ]);
?>
;
        for (map<string, int>::iterator it = intConstants.begin(); it != intConstants.end(); it++) {
            stringstream ss;
            ss << it->second;
<?php
grokit\sql_instantiate_parameters( [ '(it->first).c_str()', '(ss.str()).c_str()', '"integer"', ] );
?>
;
        }
        for (map<string, double>::iterator it = doubleConstants.begin(); it != doubleConstants.end(); it++) {
            stringstream ss;
            ss << it->second;
<?php
grokit\sql_instantiate_parameters( [ '(it->first).c_str()', '(ss.str()).c_str()', '"double"', ] );
?>
;
        }
        for (map<string, string>::iterator it = stringConstants.begin(); it != stringConstants.end(); it++) {
<?php
grokit\sql_instantiate_parameters( [ '(it->first).c_str()', '(it->second).c_str()', '"string"', ] );
?>
;
        }
<?php
grokit\sql_parametric_end();
?>
;

<?php
grokit\sql_statements_norez( <<<'EOT'
"
        DELETE FROM CatalogRelations ;
        "
EOT
, [ ] );
?>
;

<?php
grokit\sql_statements_norez( <<<'EOT'
"
        DELETE FROM CatalogAttributes;
        "
EOT
, [ ] );
?>
;

<?php
grokit\sql_statements_norez(<<<'EOT'
"
  DELETE FROM CatalogClusters;
"
EOT
, []);
?>
;

    schemas.MoveToStart();
    int _relID = 0;
    while (schemas.RightLength()) {
        Schema& sch = schemas.Current();
        string relName = sch.GetRelationName();
        int numTup = sch.GetNumTuples();
        AttributeContainer attCont;
        sch.GetAttributes(attCont);
        Attribute clusterAtt;
        bool clustered = sch.GetClusterAttribute(clusterAtt);

<?php
grokit\sql_statement_parametric_norez( <<<'EOT'
"
            INSERT INTO CatalogRelations(relation, relID, size) VALUES (?1, ?2, ?3);
            "
EOT
, [ 'text', 'int', 'int', ], [ ]);
?>
;
            _relID++;
<?php
grokit\sql_instantiate_parameters( [ '(relName).c_str()', '_relID', 'numTup', ] );
?>
;
<?php
grokit\sql_parametric_end();
?>
;

<?php
grokit\sql_statement_parametric_norez( <<<'EOT'
"
                        INSERT INTO CatalogAttributes(colNo, relID, type, jType, attribute, uniqueVals) VALUES (?1, ?2, ?3, ?4, ?5, ?6);
                        "
EOT
, [ 'int', 'int', 'text', 'text', 'text', 'int', ], [ ]);
?>
;
            attCont.MoveToStart();
            while (attCont.RightLength()) {
                Attribute& att = attCont.Current();
                int _id = att.GetIndex();
                long int _uniq = att.GetUniques();
                string typ = att.GetType();
                string na = att.GetName();
                Json::FastWriter jWrite;
                string jType = jWrite.write(att.GetJType());
<?php
grokit\sql_instantiate_parameters( [ '_id', '_relID', '(typ).c_str()', '(jType).c_str()', '(na).c_str()', '_uniq', ] );
?>
;
                attCont.Advance();
            }
<?php
grokit\sql_parametric_end();
?>
;

    if( clustered ) {
<?
grokit\sql_statement_parametric_norez(<<<'EOT'
"
  INSERT INTO CatalogClusters(relID, attID)
  VALUES (?1, ?2)
"
EOT
, ['int', 'int']);
?>

    int _attId = clusterAtt.GetIndex();

<?  grokit\sql_instantiate_parameters(['_relID', '_attId']); ?>

<?  grokit\sql_parametric_end(); ?>
    }

    schemas.Advance();
    }
<?php
grokit\sql_close_database();
?>
;

  cout << "Catalog saved " << endl;

}
