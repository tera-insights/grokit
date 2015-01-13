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
#include "CoordinatorImp.h"
#include "Catalog.h"
#include "Translator.h"
#include "DPMessages.h"
#include "CodeLoader.h"
#include "CLMessages.h"
#include "TransMessages.h"
#include "ExecEngine.h"
#include "EEExternMessages.h"
#include "DiskPool.h"
#include "Column.h"
#include "MmapAllocator.h"
#include "MMappedStorage.h"
#include "NumaMemoryAllocator.h"

#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <libgen.h>
#include <string>

using namespace std;

bool CoordinatorImp :: quitWhenDone = false;

CoordinatorImp::CoordinatorImp(bool _quitWhenDone, bool batchMode, bool compileOnly)
#ifdef DEBUG_EVPROC
    : EventProcessorImp(true, "Coordinator") // comment to remove debug
#endif

{
    this->compileOnly = compileOnly;

    execEngine.copy(executionEngine /* global var; how ugly is that*/);

    // Start the translator
    Translator tr(myInterface, batchMode);
    translator.swap(tr);
    translator.ForkAndSpin();

    // Start the code generator
    CodeLoader cg("./", myInterface);
    codeLoader.swap(cg);
    codeLoader.ForkAndSpin();

    // we register the messges since we need to receive some
    RegisterMessageProcessor(DieMessage::type, &DieProc, 1);
    RegisterMessageProcessor(QueriesDoneMessage::type, &QueryFinishedProc, 1);
    RegisterMessageProcessor(SymbolicQueryDescriptions::type, &SymbolicQueriesProc, 2);
    RegisterMessageProcessor(LoadedCodeMessage::type, &CodeProc, 3);
    RegisterMessageProcessor(NewPlan::type, &NewPlanProc, 4);

    CoordinatorImp :: quitWhenDone = _quitWhenDone;

}

void CoordinatorImp :: Quit() {
    globalDiskPool.Stop();

    // killing the execution engine
    KillEvProc(execEngine);
    // killing the translator
    KillEvProc(translator);
    // killing the code generator
    KillEvProc(codeLoader);

    // killing ourselves
    Seppuku();
}

MESSAGE_HANDLER_DEFINITION_BEGIN(CoordinatorImp, SymbolicQueriesProc,
        SymbolicQueryDescriptions){

    // take over the graph
    evProc.cachedGraph.swap(msg.newGraph);

    evProc.newTasks.SuckUp(msg.tasks);

    string lastDir(evProc.lastDir);
    LoadNewCodeMessage_Factory(evProc.codeLoader, lastDir, msg.wpDesc);

}MESSAGE_HANDLER_DEFINITION_END


MESSAGE_HANDLER_DEFINITION_BEGIN(CoordinatorImp, CodeProc, LoadedCodeMessage){

    if( evProc.compileOnly ) {
        evProc.Quit();
    } else {
        // allright. we got the code, we have the graph from the previous message
        // so we are ready to tell the execution engine about the new battle plan
        ConfigureExecEngineMessage_Factory (evProc.execEngine, evProc.cachedGraph,
                msg.configs, evProc.newTasks);

        // NOTE: the file scanners were configured by the Translator
    }


}MESSAGE_HANDLER_DEFINITION_END


MESSAGE_HANDLER_DEFINITION_BEGIN(CoordinatorImp, DieProc, DieMessage){
    cerr << "============= TAKING THE SHOW DOWN ================" << endl;
    evProc.Quit();
}MESSAGE_HANDLER_DEFINITION_END


MESSAGE_HANDLER_DEFINITION_BEGIN(CoordinatorImp, NewPlanProc, NewPlan){
    // got an xml file. Send it to the translator

    // we now sent the message to the translator
    TranslationMessage_Factory(evProc.translator, msg.confFile);

    char buffer[10000];

    strcpy(buffer, msg.confFile.c_str());
    strcpy(evProc.lastDir, dirname(buffer));

    cout << "Got some queries started from " << buffer <<" \a\aat TIME=" << evProc.clock.GetTime() << endl;


}MESSAGE_HANDLER_DEFINITION_END



MESSAGE_HANDLER_DEFINITION_BEGIN(CoordinatorImp, QueryFinishedProc, QueriesDoneMessage){
    // the exec engine finished, we take the show down

    cout << "Got some completed queries! \a\aat TIME=" << evProc.clock.GetTime() << endl;

    MMAP_DIAG;

    msg.completedQueries.MoveToStart ();
    while (msg.completedQueries.RightLength ()) {
        msg.completedQueries.Current ().exit.Print ();
        msg.completedQueries.Current ().query.Print ();
        msg.completedQueries.Advance ();
    }

    if( CoordinatorImp :: quitWhenDone ) {
        sleep(2);
        exit(EXIT_SUCCESS);
    }

}MESSAGE_HANDLER_DEFINITION_END
