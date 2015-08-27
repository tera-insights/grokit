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
/** Datapath main program.

  All we have to do is start the coordinator then start talking to
  the planner through the pipe EXECUTE. We do the talking in the
  main program since it does not fit the EventProcessor model and
  the main program has nothing better to do anyways (ways mostly).
  */

#include <algorithm>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <string>
#include <iostream>
#include <unistd.h>
#include <cinttypes>

#include <sys/resource.h>

#include "Errors.h"
#include "Message.h"
#include "Coordinator.h"
#include "MetadataDB.h"
#include "DiskArray.h"
#include "Logging.h"
#include "Constants.h"
#include "DPMessages.h"
#include "ExecEngine.h"
#include "CPUWorkerPool.h"
#include "Timer.h"
#include "Diagnose.h"
#include "Profiler.h"
#include "PCProfiler.h"
#include "PerfTopProfiler.h"
#include "ExternalCommands.h"
#include "CommunicationFramework.h"

#include "Catalog.h"

#define DEBUG

using namespace std;

// all of the event global event processor classes go here... note that since I
// just have a "pretend" disk setup (where chunks are created entirely using CPU)
// I use CPU workers to handle the "disk"... obviously, this will change when the
// "real" disk is added in
ExecEngine executionEngine("engine");
CPUWorkerPool myCPUWorkers (NUM_EXEC_ENGINE_THREADS);
CPUWorkerPool myDiskWorkers (0); // no CPU based disk workers

// used by done queries, must be global
EventProcessor globalCoordinator;

int main(int argc, char** argv){
    // variables to control behavior of datapath
    bool rdOnly=false; // run in readonly mode
    char* progToRun = NULL; // if null , we open a pipe instead of run a program
    bool isDaemon=false; // run  as demon and listen to the EXECUTE pipe
    bool quitWhenDone=false; // quit after completing queries
    bool suppressOutput = false; // Suppress profiler output
    bool batchMode = false;
    bool compileOnly = false;

    const rlim_t kStackSize = 64L * 1024L * 1024L;  // min stack size = 64 MB
    rlimit rl;
    int result;

    if (argc == 1) {
        cout << "dp: main DataPath executable" << endl;
        cout << "Command line options:" << endl;
        cout << "\t-d\t run as deamon, listen on the EXECUTE pipe for commands" << endl;
        cout << "\t-e program\t execute program then exit" << endl;
        cout << "\t-r\t run in read-only mode, no changes to data on disk" << endl;

        return 1;
    }

    external_commands_init();
    opterr = 0;

    // Attempt to increase stack size
    result = getrlimit(RLIMIT_STACK, &rl);
    if (result == 0)
    {
        if (rl.rlim_cur < kStackSize &&
            (rl.rlim_max == RLIM_INFINITY || rl.rlim_max >= kStackSize))
        {
            rl.rlim_cur = (rl.rlim_max == RLIM_INFINITY) ? kStackSize :
                std::min(kStackSize, rl.rlim_max);

            result = setrlimit(RLIMIT_STACK, &rl);
            if (result != 0)
            {
                fprintf(stderr, "error: setrlimit returned result = %d\n", result);
            } else {
                printf("stack size set to %ld KiB\n", rl.rlim_cur / 1024L);
            }
        }
    }

    int c;
    while ((c = getopt (argc, argv, "bde:rqost")) != -1){
        switch(c){
            case 'b': batchMode=true; break;
            case 'd': isDaemon=true; break;
            case 'e': progToRun=optarg; break;
            case 'r': rdOnly=true; break;
            case 'q': quitWhenDone=true; break;
            case 's': suppressOutput = true; break;
            case 't': compileOnly = true; break;
            case '?':
                      if (optopt == 'e')
                          fprintf (stderr, "Option -%c requires an argument.\n", optopt);
                      else if (isprint (optopt))
                          fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                      else
                          fprintf (stderr,
                                  "Unknown option character `\\x%x'.\n",
                                  optopt);
                      return 1;
            default:
                      abort ();
        }
    }


    if (progToRun!=NULL && isDaemon){
        cout << "Cannot execute a program and be a daemon as well. Willnot run " << progToRun << endl;
        progToRun = NULL;
    }

    if( isDaemon && quitWhenDone ) {
        cout << "Ignoring option to quit when done because we are starting as a daemon." << endl;
        quitWhenDone = false;
    }


    if (isDaemon){
        cout << "Starting Datapath as daemon. Make sure somebody writes on pipe EXECUTE" << endl;
    } else if (progToRun == NULL){
        cout << "You must either run as daemon or execute a program" << endl;
        return 1;
    }

    // starting the logging
    StartLogging();

    LOG_ENTRY(1, "MAIN: Datapath Started");

    const uint16_t listenPort = 11111;
    StartCommunicationFramework(listenPort);
    LOG_ENTRY(1, "MAIN: Communication Framework Started");

    // Make the Catalog send a schema update
    if( !batchMode ) {
        Catalog& catalog = Catalog::GetCatalog();
        catalog.SendUpdate();
    }

    // starting the profiler
    globalProfiler.SuppressOutput(suppressOutput);
    globalProfiler.ForkAndSpin();
    LOG_ENTRY(1, "MAIN: Global Profiler Started");

    //  diagnose.StartDiagnose();

    DiskArray::LoadDiskArray(rdOnly);
    LOG_ENTRY(1, "MAIN: Disk Array Loaded");

    PCProfiler pcprof(globalProfiler);
    pcprof.Run();
    LOG_ENTRY(1, "MAIN: Profiler Heartbeat Started");

    //PerfTopProfiler perfTopProf(globalProfiler);
    //perfTopProf.Run();
    //LOG_ENTRY(1, "MAIN: Perf Top Profiler Started");

    // start the execution engine
    executionEngine.ForkAndSpin ();
    // start the coordinator. Exec engine already ticking
    Coordinator coord(quitWhenDone, batchMode, compileOnly);
    globalCoordinator.swap(coord);
    globalCoordinator.ForkAndSpin();

    if (progToRun != NULL){
        // we got an argument. We just run the file and die
        string tProg(progToRun);
        NewPlan_Factory(globalCoordinator, tProg);
    } else {
        // we get our instructions from EXECUTE pipe
        // main interactionloop with the planner
        // we read from the pipe EXECUTE until the word QUIT commes, when we
        // take Datapath down
        FILE* fd = fopen("EXECUTE", "r");

        while (true){
            char fileName[1000];
            //printf("Insert the file name\n");
            if (fscanf(fd, "%s", fileName)!=1){
                WARNING("Something funny going on with the pipe connecting datapath to the planner\n"
                        "You probably killed the Planner. START IT AGAIN, FAST. Sleeping for 1 seconds");


                sleep(1);
                fclose(fd);
                FILE* fd = fopen("EXECUTE", "r");
                continue;
            }

            if (strcasecmp(fileName, "QUIT") == 0){
                globalCoordinator.Seppuku(); // send the kill message to the globalCoordinator
                break; // we are shutting down Datapath
            }

            cout << "Got the file " << fileName << " from the Planner. Passing it along" << endl;
            // we have a file name. Send it to the coordinator
            string tFileName(fileName);
            NewPlan_Factory(globalCoordinator, tFileName);
        }
    }

    // the thread running the main program is still available
    // we block it until the file writer dies
    globalCoordinator.WaitForProcessorDeath();

    // stop the diagnosis
    //	diagnose.StopDiagnose();
    globalProfiler.Seppuku();

    StopCommunicationFramework();

    // stop the logging
    StopLogging();

    return 0;
}
