// Copyright 2013 Tera Insights, LLC. All Rights Reserved.
// Author: Christopher Dudley

// FIXME: This currently doesn't work (well) because of STDIO's output buffering.
// The profiler doesn't get any output until the 4096 byte buffer fills up in
// the Perf process.

#include "PerfTopProfiler.h"
#include "Errors.h"
#include "Logging.h"
#include "SerializeJson.h"
#include "PerfProfMSG.h"

#include <string>
#include <sstream>
#include <array>

#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cmath>
#include <ctime>

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

namespace {
    enum State {
        BEGIN = 0
        , STATUS_LINE
        , FILLER1
        , FILLER2
        , INFO_LINE
        , END

        , ERROR
    };

    const std::array<std::string, 7> stateNames = {
        "BEGIN"
        , "STATUS_LINE"
        , "FILLER1"
        , "FILLER2"
        , "INFO_LINE"
        , "END"
        , "ERROR"
    };
}

PerfTopProfiler :: PerfTopProfiler(EventProcessor& _profiler) {
    evGen = new PerfTopProfilerImp(_profiler);
}

PerfTopProfilerImp :: PerfTopProfilerImp(EventProcessor & _profiler) :
    myProfiler(),
    childID(-1),
    childIn(-1),
    childOut(-1),
    childErr(-1),
    buffer(),
    start(buffer),
    end(buffer)
{
    myProfiler.copy(_profiler);
}

void PerfTopProfilerImp :: PreStart(void) {
    // Start up the Perf Top process.
    pid_t pid = getpid();

    // Create pipes
    int inPipe[2];      // Input to child
    int outPipe[2];     // Output from child
    int errPipe[2];     // Error from child

    if( pipe(inPipe) != 0 || pipe(outPipe) != 0 || pipe(errPipe) != 0 ) {
        perror("PerfTopProfiler pipe");
        FATAL("PerfTopProfiler unable to create pipes");
    }

    pid_t childid = fork();

    if( childid == -1 ) {
        perror("PerfTopProfiler fork");
        FATAL("PerfTopProfiler unable to fork");
    }

    if( childid > 0 ) {
        // Parent
        // Close read end of inPipe and write end of outPipe
        close(inPipe[0]);
        close(outPipe[1]);
        close(errPipe[1]);

        childID = childid;
        childIn = inPipe[1];
        childOut = outPipe[0];
        childErr = errPipe[0];
    }
    else {
        // Child
        // Close write end of inPipe and read end of outPipe
        close(inPipe[1]);
        close(outPipe[0]);
        close(errPipe[0]);

        // Duplicate inpipe to STDIN
        //dup2(inPipe[0], 0);
        // Duplicate outpipe to STDOUT
        dup2(outPipe[1], 1);
        // Duplicate errPipe to STDERR
        dup2(errPipe[1], 2);

        std::ostringstream ss;
        ss << pid;
        const char * pidString = ss.str().c_str();

        // Brain swap
        execlp("stdbuf", "stdbuf", "--output=0", "perf", "top", "-K", "--percent-limit", "0.01", "--stdio", "-z", "-p", pidString);

        // If we got here, something went wrong
        perror("PerfTopProfiler execlp");
        exit(EXIT_FAILURE);
    }

    /*
    std::ostringstream cmdBuilder;
    cmdBuilder << "stdbuf --output=0 perf top -K --percent-limit 0.01 --stdio -z -p " << pid;
    const char * command = cmdBuilder.str().c_str();

    perfOut = popen(command, "r");

    if( perfOut == NULL ) {
        perror("PerfTopProfiler popen");
        FATAL("PerfTopProfiler unable to start child process");
    }
    */
}

int PerfTopProfilerImp :: ProduceMessage() {
    //int fd = fileno(perfOut); // file descriptor for select()
    int fd = childOut;

    //if( fd == -1 ) {
        //perror("PerfTopProfiler fileno");
        //return -1;
    //}

    //printf("PerfTopProfiler :: ProduceMessage\n");

    timespec wallTime;
    clock_gettime(CLOCK_REALTIME, &wallTime);
    int64_t wallTimeMillis = (wallTime.tv_sec * 1000) + (wallTime.tv_nsec / 1000000);

    fd_set rfds;              // set of file descriptors
    timeval timeout;          // Timeout for select()
    int retval = 0;           // Return value of select()
    const char * bEnd = buffer + BUFFER_SIZE - 1;   // one past the end for the entire buffer
    char * nLoc = NULL;    // Location of first newline from start
    Json::Value info(Json::objectValue);    // Information to send
    Json::Value content(Json::arrayValue);  // List of function performance infos

    // Variables for extracting information from lines
    int scanRet = 0;
    const char * statusPattern = " PerfTop: %ld irqs/sec kernel: %lf%% exact: %lf%% [%ldHz cycles]";
    long irqs = 0, freq = 0;
    double kernel = 0.0, exact = 0.0;

    double percentage = 0.0;

    State prevState = BEGIN;
    State state = BEGIN;
    State nextState = BEGIN;
    State errState = BEGIN;

    constexpr char ESC = 0x1B;

    bool readMore = true;       // Whether or not to continue reading input

    // Watch the file descriptor to check for output
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    // Timeout of 100ms
    timeout.tv_sec = 0;
    timeout.tv_usec = 100 * 1000;

    while( readMore && (retval = select(fd+1, &rfds, NULL, NULL, &timeout)) > 0 ) {
        // If retval > 0, then fd must be ready (it's the only fd)

        // Try to fill up the buffer
        end += read(fd, end, bEnd - end);
        *end = '\0'; // Make sure string is null terminated

        while( readMore && (nLoc = strchr(start, '\n')) != NULL ) {
            // Switch newline to null
            *nLoc = '\0';

            switch( state ) {
                case BEGIN:
                    // this line should start with an escape
                    if( *start == ESC ) {
                        // Advance to next line
                        start = nLoc + 1;
                        nextState = STATUS_LINE;
                    }
                    else {
                        nextState = ERROR;
                    }
                    break;
                case STATUS_LINE:
                    LOG_ENTRY_P(2, "Status Line: %s\n", start);
                    scanRet = sscanf(start, statusPattern, &irqs, &kernel, &exact, &freq);
                    if( scanRet == 4 ) {
                        // All values read successfully
                        nextState = FILLER1;
                        start = nLoc + 1;

                        info["irqs"] = (Json::Int64) irqs;
                        info["kernel"] = (Json::Int64) lround(kernel * 10);
                        info["exact"] = (Json::Int64) lround(exact * 10);
                        info["freq"] = (Json::Int64) freq;
                    }
                    else {
                        nextState = ERROR;
                    }
                    break;
                case FILLER1:
                    LOG_ENTRY_P(2, "Filler1: %s\n", start);
                    // Skip line
                    start = nLoc + 1;
                    nextState = FILLER2;
                    break;
                case FILLER2:
                    LOG_ENTRY_P(2, "Filler2: %s\n", start);
                    // Skip line
                    while( start < nLoc && std::isspace(*start) )
                        start++;

                    if( start == nLoc ) {
                        start = nLoc + 1;
                        nextState = FILLER2;
                    }
                    else {
                        *nLoc = '\n';
                        nextState = INFO_LINE;
                    }
                    break;
                case INFO_LINE:
                    if( *start == ESC ) {
                        // New frame started
                        nextState = END;
                    }
                    else {
                        LOG_ENTRY_P(2, "Info Line: %s\n", start);
                        char * libName = NULL;
                        char * libNameEnd = NULL;
                        char * funcName = NULL;
                        char * funcNameEnd = NULL;

                        // Remove whitespace before percentage
                        while( std::isspace(*start) ) start++;
                        percentage = strtod(start, NULL);

                        // Find next whitespace
                        while( ! std::isspace(*start) ) start++;

                        // Skip the whitespace
                        while( std::isspace(*start) ) start++;

                        // We're now at the library name
                        libName = start;

                        // Find next whitespace
                        while( ! std::isspace(*start) ) start++;
                        // Make an end of string for the library name
                        libNameEnd = start;
                        *libNameEnd = '\0';
                        start++;

                        // Skip the whitespace
                        while( std::isspace(*start) ) start++;

                        // This is the [.]
                        // Skip it
                        while( ! std::isspace(*start) ) start++;

                        funcName = start;

                        // Trim whitespace on the right.
                        funcNameEnd = nLoc - 1;
                        while( std::isspace(*funcNameEnd) ) funcNameEnd--;
                        funcNameEnd++;
                        *funcNameEnd = '\0';

                        // We're at the function name
                        Json::Value infoLine(Json::objectValue);
                        infoLine["portion"] = (Json::Int64) lround(percentage * 10);
                        infoLine["library"] = Json::Value(libName, libNameEnd);
                        infoLine["function"] = Json::Value(funcName, funcNameEnd);

                        content.append(infoLine);

                        // Advance to next line
                        start = nLoc + 1;
                        nextState = INFO_LINE;
                    }
                    break;
                case END:
                    readMore = false;
                    break;
                case ERROR:
                    errState = prevState;
                    readMore = false;
                    break;
            }

            prevState = state;
            state = nextState;
        }

        // Move the remaining buffer to the beginning
        off_t rSize = end - start;
        memmove( buffer, start, rSize );
        start = buffer;
        end = start + rSize;
        *end = '\0';
    }

    if( retval < 0 ) {
        perror("PerfTopProfiler select()");
        FATAL("PerfTopProfiler call to select() failed");
    }

    // We are either here because the timeout expired (there's no more data for this frame)
    // or we reached the next frame, so send the message.

    if( state != ERROR ) {
        if( ! info.empty() ) {
            // Frame isn't empty
            info["content"] = content;

            PerfTopMessage::Factory(myProfiler, wallTimeMillis, info);
        }
    }
    else {
        LOG_ENTRY_P(1, "PerfTopProfiler failed to parse output in state %s, buffer:\n%s",
            stateNames[errState].c_str(), start );
    }

    return 0;
}
