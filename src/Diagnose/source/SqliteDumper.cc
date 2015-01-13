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
#include <iostream>
#include <cstring> //strlen

#include "Diagnose.h"

using namespace std;

SqliteDumperImp::SqliteDumperImp(void)
#ifdef DEBUG_EVPROC
: EventProcessorImp(true, "SqliteDumper") // comment to remove debug
#endif
{
	// register the DumpRequestMessage
	RegisterMessageProcessor(DumpRequestMessage::type, &DumpRequest, 1 /*priority*/);

	//open database connection.
	//TODO: Check if database is blocked
	int rc = sqlite3_open("log.sqlite", &logDB);
	char *zErrMsg = 0; //error message is passed via this

	if(rc){ //in case of an error
		cerr<< "Can't open database: " <<sqlite3_errmsg(logDB) << endl;
	  	sqlite3_close(logDB);
	  	return;
	}

	rc = sqlite3_exec(logDB, "PRAGMA journal_mode=WAL", NULL, NULL, &zErrMsg);
				if(rc){ //in case of an error
					cerr<< "pragma error: " <<sqlite3_errmsg(logDB) << endl;
				  	sqlite3_close(logDB);
				  	exit(1);
				}



	//prepare a pre-compiled INSERT INTO statement. Helps in improving performance
	zSQL_INS =  "INSERT INTO LOG_RECORDS VALUES (@ID, @SENDER, @DEST, @MSGTYPE, @TS, @DURATION);";
	sqlite3_prepare_v2(logDB, zSQL_INS, strlen(zSQL_INS), &preStmtINS, NULL);

	//prepare a pre-compiled UPDATE statement
	//special case of using DIAGNOSE_EXIT
	//update duration using currentTimeStamp - OldTimeStamp
	zSQL_UPD =  "UPDATE LOG_RECORDS SET DURATION = @NEWTS - (SELECT TIMESTAMP FROM LOG_RECORDS WHERE LOG_ID = @ID) WHERE LOG_ID = @ID;";
	sqlite3_prepare_v2(logDB, zSQL_UPD, strlen(zSQL_UPD), &preStmtUPD, NULL);

	//creating LOG_RECORDS table if it does not exists
	rc = sqlite3_exec(logDB, "CREATE TABLE IF NOT EXISTS LOG_RECORDS (LOG_ID INTEGER PRIMARY KEY, SENDET TEXT, RECEIVER TEXT, MSG_TYPE TEXT, TIMESTAMP REAL, DURATION REAL);", NULL, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
		cerr<< "SQL error: " << zErrMsg << endl;
		sqlite3_free(zErrMsg);
	}

	//emptying LOG_RECORDS table for first time use
	rc = sqlite3_exec(logDB, "DELETE FROM LOG_RECORDS;", NULL, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
		cerr<< "SQL error: " << zErrMsg << endl;
		sqlite3_free(zErrMsg);
	}
}

void SqliteDumperImp::DumpToSQLite(MessageContainer& _msgContainer, bool killMe){
	//error handler objects
	int rc;	char *zErrMsg = 0;

	//Used a transaction instead of separately inserting every statement.
	//Helps tremendously in improving performance (BY THE FACTOR OF 25k ).
	sqlite3_exec(logDB, "BEGIN TRANSACTION", NULL, NULL, &zErrMsg);
	for (int i=0; i< _msgContainer.Size(); i++)
	{
		//bind values to the preparedStatement
		sqlite3_bind_int (preStmtINS, 1, _msgContainer.Get(i)->msgID);
		sqlite3_bind_text(preStmtINS, 2, _msgContainer.Get(i)->msgSender, -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(preStmtINS, 3, _msgContainer.Get(i)->msgDest, -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(preStmtINS, 4, _msgContainer.Get(i)->msgType, -1, SQLITE_TRANSIENT);
		sqlite3_bind_double(preStmtINS, 5, _msgContainer.Get(i)->msgTimeStamp);
		sqlite3_bind_double(preStmtINS, 6, -1.0);		//dummyDuration

		//execute statement
		rc = sqlite3_step(preStmtINS);
		if( rc == SQLITE_CONSTRAINT ){
			//special case of using DIAGNOSE_EXIT
			//update duration using msgTimeStamp - timestamp
			sqlite3_bind_double(preStmtUPD, 1, _msgContainer.Get(i)->msgTimeStamp);
			sqlite3_bind_int (preStmtUPD, 2, _msgContainer.Get(i)->msgID);
			sqlite3_bind_int (preStmtUPD, 3, _msgContainer.Get(i)->msgID);

			int uc = sqlite3_step(preStmtUPD);

			if(uc != SQLITE_DONE){
				cerr<< "SQL error: " << sqlite3_errmsg(logDB) <<" at LOG_ID = "<< _msgContainer.Get(i)->msgID << endl;
			}
			sqlite3_clear_bindings(preStmtUPD); //clear all these bindings for further use
			sqlite3_reset(preStmtUPD); //reset the prepared statement for further use
		}
		sqlite3_clear_bindings(preStmtINS); //clear all these bindings for further use
		sqlite3_reset(preStmtINS); //reset the prepared statement for further use
	}

	sqlite3_exec(logDB, "END TRANSACTION", NULL, NULL, &zErrMsg);

	_msgContainer.Clear();

	//if killMe is set to true, that means this message is sent from Diagnose destructor and contains last few message records
	//hence now delete this EventProcessor after closing the database connection
	if(killMe){
		sqlite3_close(logDB);
		Seppuku();
	}
}

// DumpRequest handler definition
MESSAGE_HANDLER_DEFINITION_BEGIN(SqliteDumperImp, DumpRequest, DumpRequestMessage){
	evProc.DumpToSQLite(msg.msgContainer, msg.killMe);
}MESSAGE_HANDLER_DEFINITION_END
