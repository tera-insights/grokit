// To be included by module
#include <iostream>
#include <string>
#include <vector>

#include <boost/tokenizer.hpp>

/*
 * GI_DESC
 *  NAME(NetflowCSVReader)
 *  OUTPUTS(</(ip, IPV4ADDR), (flow, INT)/>)
 *
 * END_DESC
 */

class NetflowCSVReader {
    using namespace std;

    istream &stream;

    // Template parameters
    static const size_t HEADER_LINES = 0;
    static const char DELIMITER = ',';
    static const char ESCAPE_CHAR = '\\';
    static const char QUOTE_CHAR = '\"';

    typedef boost::escaped_list_separator<char> separator;
    typedef boost::tokenizer< separator > Tokenizer

    // Prevent having to allocate this every time.
    string line;
    vector<string> tokens;

private:

    void tokenize_line( string& line, vector<string>& tokens ) {
        tokens.clear();

        Tokenizer tok(line, separator(ESCAPE_CHAR, DELIMITER, QUOTE_CHAR) );

        for( Tokenizer::iterator i = tok.begin(); i != tok.end(); ++i ) {
            tokens.push_back(*i);
        }
    }

public:
    NetflowCSVReader( istream &_stream ) : stream(_stream) {
        for( int i = 0; i < HEADER_LINES; ++i ) {
            FATALIF( !getline( stream, line ), "CSV Reader reached end of file before finishing header.\n" );
        }
    }

    bool ProduceTuple( IPV4ADDR& ip, INT& flow ) {
        if( getline( stream, line ) ) {
            tokenize_line( line, tokens );
            size_t index = 0;

            // read in IP
            FromString( ip, tokens[index++] );
            // read in flow
            FromString( flow, tokens[index++] );

            return true;
        } else {
            return false;
        }
    }
};
