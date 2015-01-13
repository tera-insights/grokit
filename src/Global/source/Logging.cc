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
#include "Logging.h"
#include "Errors.h"

#include <stdio.h>

Timer global_clock;
FILE* logFile;

void StartLogging(void){
	printf("Starting LOGGER in file %s with LOG_LEVEL=%d\n", LOG_FILE, LOG_LEVEL);
	// start global clock
	global_clock.Restart();
	if (LOG_LEVEL>0){
        logFile = fopen(LOG_FILE, "w");
        if( logFile == NULL ) {
            perror("Unable to open log file");
            FATAL("Failed to open log file.");
        }
	}

}

void StopLogging(void){
	if (LOG_LEVEL>0){
        fclose(logFile);
	}
}
