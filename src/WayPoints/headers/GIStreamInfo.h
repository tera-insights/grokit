//
//  Copyright 2013 Christopher Dudley
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
#ifndef _GI_STREAM_INFO_H_
#define _GI_STREAM_INFO_H_

#include <iostream>
#include <fstream>
#include <string>

#include "Swap.h"
#include "Config.h"
#include "Logging.h"

// Forward declaration
class GIStreamInfo;

class GIStreamProxy {
    // Private members
    std::istream* stream;
    off_t id;
    std::string file_name;

public:
    GIStreamProxy() : stream(NULL), id(-1), file_name("No File") {
    }

private:
    GIStreamProxy( std::istream* _stream, const off_t _id, const std::string& _file_name ) : stream(_stream), id(_id), file_name(_file_name) {
    }

public:
    GIStreamProxy( const GIStreamProxy &other ) : stream(other.stream), id(other.id), file_name(other.file_name) {
    }

#ifdef _HAS_CPP_11
    GIStreamProxy( GIStreamProxy &&other ) : stream(NULL), id(-1), file_name("No File") {
        SWAP_STD(stream, other.stream);
        SWAP_STD(id, other.id);
        SWAP_STD(file_name, other.file_name)
    }
#endif // _HAS_CPP_11

    void swap( GIStreamProxy &other ) {
        SWAP_STD(stream, other.stream);
        SWAP_STD(id, other.id);
        SWAP_STD(file_name, other.file_name);
    }

    void copy( const GIStreamProxy& other ) {
        stream = other.stream;
        id = other.id;
        file_name = other.file_name;
    }

    // Return a reference to the stream.
    std::istream & get_stream( void ) {
        FATALIF( stream == NULL, "Error: Attempted to access invalid stream.");
        return *stream;
    }

    off_t get_id( void ) const {
        return id;
    }

    std::string get_file_name( void) const {
        return file_name;
    }

    bool done( void ) const {
        return (stream == NULL || !(stream->good()) );
    }

    friend class GIStreamInfo;
};

inline
void swap( GIStreamProxy& a, GIStreamProxy& b ) {
    a.swap(b);
}

class GIStreamInfo {
    typedef std::ifstream ifstream;

protected:
    static const std::streamsize BUFFER_SIZE = 1<<20;

    // Private members
    ifstream * stream; // Hacky, due to lack of swap in libstdc++
    char * buffer;
    std::string file_name;
    off_t id;

public:

    // Empty stream object.
    GIStreamInfo() : stream(NULL), buffer(NULL), file_name("No File"), id(-1) {
    }

    // Creates a stream bound to a file with a non-standard buffer.
    GIStreamInfo( const std::string& file, off_t _id ) : buffer(NULL), file_name(file),  id(_id) {
        stream = new ifstream( file.c_str() );
        if( stream->fail() ) {
            LOG_ENTRY_P(1, "Failed to open file %s", file.c_str());
        }
        else {
            buffer = new char[BUFFER_SIZE];
            stream->rdbuf()->pubsetbuf( buffer, BUFFER_SIZE );
        }
    }

    // Delete copy constructor
    GIStreamInfo( const GIStreamInfo& other ) = delete;

#ifdef _HAS_CPP_11
    // Move constructor
    GIStreamInfo( GIStreamInfo &&other ) : stream(NULL), buffer(NULL), file_name("No File"), id(-1) {
        SWAP_STD( stream, other.stream );
        SWAP_STD( buffer, other.buffer );
        SWAP_STD( file_name, other.file_name );
        SWAP_STD( id, other.id );
    }

    // Move assign
    GIStreamInfo & operator = (GIStreamInfo && other ) {
        this->swap(other);
        return *this;
    }
#endif // _HAS_CPP_11

    // Deallocate any buffer that was allocated and close open streams.
    ~GIStreamInfo( void ) {
        if( stream != NULL ) {
            if( stream->is_open() )
                stream->close();

            delete stream;
            stream = NULL;
        }

        if( buffer != NULL ) {
            delete [] buffer;
            buffer = NULL;
        }

    }

    // Return a reference to the stream.
    std::istream & get_stream( void ) {
        FATALIF( stream == NULL, "Error: Attempted to access invalid stream.");
        return *stream;
    }

    std::string get_file_name( void ) const {
        return file_name;
    }

    off_t get_id( void ) const {
        return id;
    }

    bool done( void ) const {
        return stream != NULL ? !stream->good() : true;
    }

    // Swapping paradigm
    void swap( GIStreamInfo& other ) {
        SWAP_STD( stream, other.stream );
        SWAP_STD( buffer, other.buffer );
        SWAP_STD( file_name, other.file_name );
        SWAP_STD( id, other.id );
    }

    GIStreamProxy get_proxy( void ) const {
        FATALIF( stream == NULL, "Error: Attempted to create proxy for invalid stream." );
        return GIStreamProxy( stream, id, file_name );
    }
};

// Swap function for compatibility with C++11 swap
inline
void swap( GIStreamInfo& a, GIStreamInfo& b ) {
    a.swap(b);
}

#endif // _GI_STREAM_INFO_H_
