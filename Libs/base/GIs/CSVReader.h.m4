dnl #
dnl #  Copyright 2013 Christopher Dudley
dnl #
dnl #  Licensed under the Apache License, Version 2.0 (the "License");
dnl #  you may not use this file except in compliance with the License.
dnl #  You may obtain a copy of the License at
dnl #
dnl #      http://www.apache.org/licenses/LICENSE-2.0
dnl #
dnl #  Unless required by applicable law or agreed to in writing, software
dnl #  distributed under the License is distributed on an "AS IS" BASIS,
dnl #  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
dnl #  See the License for the specific language governing permissions and
dnl #  limitations under the License.

GI_TEMPLATE_DESC(</CSVReader/>)

dnl # Arguments:
dnl # $1 = GI Name
dnl # $2 = List of output attributes (assumed to exactly match file)
dnl # #3 = (Optional) Number of lines of headers [0]
dnl # $3 = (Optional) Delimiter character [,]
dnl # $4 = (Optional) Quote character ["]
dnl # $5 = (Optional) Escape character [\]
m4_define(</CSVReader/>, </dnl
m4_divert_push(-1)

m4_redefine(</MY_NAME/>, </$1/>)
m4_redefine(</MY_OUTPUTS/>, </$2/>)

m4_ifval($3, </
    m4_redefine(</MY_HEADER_LINES/>, </$3/>)
/>,</
    m4_redefine(</MY_HEADER_LINES/>, </0/>)
/>)

m4_ifval($4, </
    m4_redefine(</MY_DELIM/>, m4_quote(m4_strip_quotes(</$4/>)))
    m4_if( m4_len(MY_DELIM), 1, <//>, </
        m4_fatal(</Error: Multi-character delimiters not supported: />MY_DELIM)
    />)
/>,</
    dnl # Default delimiter
    m4_redefine(</MY_DELIM/>, </,/>)
/>)

m4_ifval($5, </
    m4_redefine(</MY_QUOTE/>, m4_quote(m4_strip_quotes(</$5/>)))
    m4_if( m4_len(MY_QUOTE), 1, <//>, </
        m4_fatal(</Error: Multi-character quotes not supported: />MY_QUOTE)
    />)
/>,</
    dnl # Default quote
    m4_redefine(</MY_QUOTE/>, </"/>)
/>)

m4_ifval($6, </
    m4_redefine(</MY_ESCAPE/>, m4_quote(m4_strip_quotes(</$6/>)))
    m4_if( m4_len(MY_ESCAPE), 1, <//>, </
        m4_fatal(</Error: Multi-character escapes not supported: />MY_ESCAPE)
    />)
/>,</
    dnl # Default escape
    m4_redefine(</MY_ESCAPE/>, </\\/>)
/>)

m4_divert_pop(-1)<//>dnl
#include <vector>
#include <string>
#include <iostream>
#include <boost/tokenizer.hpp>

#include "Errors.h"

M4_DICTIONARY_INCLUDES

/** Information block for MY_NAME
  * GI_DESC
  *     NAME(MY_NAME)
  *     OUTPUTS(MY_OUTPUTS)
  * END_DESC
  */

class MY_NAME {
    std::istream& my_stream;

    // Template parameters
    static const size_t HEADER_LINES = MY_HEADER_LINES;
    static const char DELIMITER = 'MY_DELIM';
    static const char QUOTE_CHAR = 'MY_QUOTE';
    static const char ESCAPE_CHAR = 'MY_ESCAPE';

    typedef boost::escaped_list_separator<char> separator;
    typedef boost::tokenizer< separator > Tokenizer;

    // Prevent having to allocate this every time.
    std::string line;
    std::vector<std::string> tokens;
    separator my_separator;
    Tokenizer my_tokenizer;

    size_t count;

    // Dictionaries for types that require them
    M4_DECLARE_DICTIONARIES(m4_quote(MY_OUTPUTS))

public:

    MY_NAME ( GIStreamProxy& _stream ) :
        count(0),
        my_stream(_stream.get_stream()),
        my_separator(ESCAPE_CHAR, DELIMITER, QUOTE_CHAR),
        my_tokenizer(string(""))
    {
        for( size_t i = 0; i < HEADER_LINES; ++i ) {
            FATALIF( !getline( my_stream, line ), "CSV Reader reached end of file before finishing header.\n" );
        }
    }

    bool ProduceTuple( TYPED_REF_ARGS(MY_OUTPUTS) ) {
        if( getline( my_stream, line ) ) {
            my_tokenizer.assign( line, my_separator );
            Tokenizer::iterator it = my_tokenizer.begin();

<//>m4_foreach(</_A_/>, </MY_OUTPUTS/>, </dnl
            M4_FROMSTRING( _A_, it->c_str() )
            ++it;
/>)dnl
            ++count;
            return true;
        } else {
            return false;
        }
    }

    M4_MERGE_DICTIONARIES(m4_quote(MY_OUTPUTS))
};
/>)dnl
