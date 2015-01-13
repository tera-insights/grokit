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
#ifndef _SQLITEDUMPER_H_
#define _SQLITEDUMPER_H_

#include <sqlite3.h>

#include "EventProcessor.h"
#include "EventProcessorImp.h"
#include "Message.h"

//Implementor class for SqliteDumper
class SqliteDumperImp : public EventProcessorImp {
private:
	sqlite3 *logDB;			//pointer to the database
	sqlite3_stmt *preStmtINS;	//prepared statement, INSERT INTO syntax, only values differ.
	sqlite3_stmt *preStmtUPD;	//prepared statement, UPDATE syntax, only values differ.
	const char* zSQL_INS;		//helper for preStmt
	const char* zSQL_UPD;		//helper for preStmt
	const char* zSQL_CREATE;        //helper for create table preStmt
	SqliteDumperImp(void);

	//function that helps in dumping data into SQLite database
	//_msgContainer: A vector containing MessageData
	//killMe: If set to true, kills itself.
	void DumpToSQLite(MessageContainer& _msgContainer, bool killMe);

	// message handling function for processing DumpRequest from Diagnose
	MESSAGE_HANDLER_DECLARATION(DumpRequest);

	friend class SqliteDumper;
};

// Interface class
class SqliteDumper : public EventProcessor {
 public:
	SqliteDumper(void);
};

inline SqliteDumper::SqliteDumper(void){
	evProc = new SqliteDumperImp();
}

#endif // _SQLITEDUMPER_H_
