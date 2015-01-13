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
#ifndef _ERRORS_H_
#define _ERRORS_H_

#include <cstdio>
#include <cstdlib>
#include <cassert>

#include <mutex>
#include <memory>

#include "json.h"

class GrokitErrorFile {
private:
    const char * path;

    std::mutex m_output;

    GrokitErrorFile(const char * errFilePath);

    void DoWrite(Json::Value &);

public:
    ~GrokitErrorFile();

    void WriteError(const char * msg, ...);
    void WriteErrorSys(const char * msg, ...);

    // Static members
    static constexpr const char * DefaultPath = "./grokit_error.json";
    static constexpr std::size_t MaxMessageLength = 16384;
private:

    // Singleton instance
    static std::unique_ptr<GrokitErrorFile> instance;
    static std::mutex m_instance;

public:
    static GrokitErrorFile & GetInstance(void);
};

/**
  All the macros defined below behave like a printf. They take a
  variable nubmer of arguments and can use the special printf facilities.

  For example
  FATAL("Something is wrong in file %s\n",fileName)
  would print the place where the error happened, then the desired
  information then exit.
*/

/* fail but write the system error as well */
#define SYS_FATAL(msg...){\
    std::fprintf(stderr, "FATAL [%s:%d] ", __FILE__, __LINE__);\
    std::fprintf(stderr, msg);\
    std::fprintf(stderr, "\n");\
    perror(NULL);\
    GrokitErrorFile::GetInstance().WriteErrorSys(msg); \
    assert(1==2);\
    exit(-1);\
}


/* macro to halt the program and print a desired message */
#define FATAL(msg...) {\
    std::fprintf(stderr, "FATAL [%s:%d] ", __FILE__, __LINE__);\
    std::fprintf(stderr, msg);\
    std::fprintf(stderr, "\n");\
    GrokitErrorFile::GetInstance().WriteError(msg); \
    assert(1==2);\
    exit(-1);\
}


/* macro to halt the program if a condition is satisfied */
#define FATALIF(expr,msg...) {\
    if (expr) {\
        FATAL(msg) \
    }\
}


/* macro to print a warning */
/* the errors are sent to standard error */
#define WARNING(msg...) {\
    std::fprintf(stderr, "WARNING [%s:%d] ", __FILE__, __LINE__);\
    std::fprintf(stderr, msg);\
    std::fprintf(stderr, "\n");\
    std::fflush(stderr); \
}


/* macro to print a warning if a condition is satisfied */
#define WARNINGIF(expr,msg...) {\
    if (expr) {\
        WARNING(msg) \
    }\
}

#endif //_ERRORS_H_
