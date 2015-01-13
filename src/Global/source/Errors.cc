// Copyright 2014 Tera Insights, LLC. All Rights Reserved.

#include <climits>
#include <cstdlib>
#include <cstdarg>
#include <cstring>

#include <iostream>
#include <fstream>

#include <errno.h>

#include "Errors.h"

std::unique_ptr<GrokitErrorFile> GrokitErrorFile::instance(nullptr);
std::mutex GrokitErrorFile::m_instance;

GrokitErrorFile & GrokitErrorFile::GetInstance(void) {
    std::unique_lock<std::mutex> guard(m_instance);

    if( !instance ) {
        instance.reset(new GrokitErrorFile(DefaultPath));
    }

    return *instance;
}

GrokitErrorFile::GrokitErrorFile(const char * errFilePath) :
    path(nullptr),
    m_output()
{
    path = realpath(errFilePath, NULL);
}

GrokitErrorFile::~GrokitErrorFile() {
    free((void*) path);
}

void GrokitErrorFile::DoWrite(Json::Value & val) {
    std::unique_lock<std::mutex> guard(m_output);

    std::ifstream curFile(path);
    if( curFile.good() ) {
        Json::Reader reader;
        reader.parse(curFile, val["__previous__"]);
    }
    curFile.close();

    std::ofstream out(path);

    Json::FastWriter writer;
    out << writer.write(val);

    out.close();
}

void GrokitErrorFile::WriteError(const char * msg, ...) {
    va_list arguments;
    char buffer[MaxMessageLength+1];

    va_start(arguments, msg);
    vsnprintf(buffer, MaxMessageLength+1, msg, arguments);
    va_end(arguments);

    Json::Value js;
    js["__type__"] = "error";
    js["kind"] = "engine";
    js["message"] = buffer;

    DoWrite(js);
}

void GrokitErrorFile::WriteErrorSys(const char * msg, ...) {
    va_list arguments;
    char buffer[MaxMessageLength+1];

    va_start(arguments, msg);
    vsnprintf(buffer, MaxMessageLength+1, msg, arguments);
    va_end(arguments);

    Json::Value js;
    js["__type__"] = "error";
    js["kind"] = "system";
    js["message"] = buffer;
    js["errno"] = Json::Int(errno);
    js["err_str"] = strerror_r(errno, buffer, MaxMessageLength+1);

    DoWrite(js);
}
