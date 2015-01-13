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
/** Global logging facilities in Datapath
*/

/** At compile time, a LOG_LEVEL can be specified. The implementation
	* of the log level is call specifig, i.e. evey log entry can specify
	* it's log level
	* entries with log levels larger than specified LOG_LEVEL will not be
	* logged
	* If LOG_LEVEL is 0, no logging happens
	*
	* LOG_FILE can be overrode at compile time. Defaulst with "LOG" in
	* the current directory.
	*
	* Two macros are offered: LOG_ENTRY and LOG_ENTRY_P
	* LOG_ENTRY_P assumes presence of extra parameters and works like a printf
	*/

#ifndef _LOGGING_H_
#define _LOGGING_H_

#include "Timer.h" // for the global clock
#include "Constants.h"

#include <stdio.h> // for fprintf and fopen

extern Timer global_clock; // the global clock
extern FILE* logFile;

// functions to start and stop the logging
void StartLogging(void);
void StopLogging(void);


// macro to log an entry (works like printf)
#define LOG_ENTRY_P(level, pattern, args...)															\
	if (level<=LOG_LEVEL && level>0){																			\
		fprintf(logFile, "%4.5f\t" pattern "\n", global_clock.GetTime(), args);	\
		fflush(logFile);																										\
	}																																			\

#define LOG_ENTRY(level, pattern)															\
	if (level<=LOG_LEVEL && level>0){																			\
		fprintf(logFile, "%4.5f\t" pattern "\n", global_clock.GetTime());	\
		fflush(logFile);																										\
	}																									
// profiling macros. They go into the same file but are fixed format

#define PROFILING(start_time, waypoint, type, pattern, args...)		\
  fprintf(logFile, "%4.5f\t%4.5f\t%s\t%s\t" pattern "\n", global_clock.GetTime(), \
	  start_time, waypoint, type, args);				\

#endif // _LOGGING_H_
