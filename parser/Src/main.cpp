#include <CommonStd.h>
#include <Tokenizer.h>

namespace LsplParser {

///////////////////////////////////////////////////////////////////////////////

class IMessageCallback {
public:
	virtual void OnMessage( const string& messageText ) = 0;
	virtual void OnWarning( const string& warningText ) = 0;
	virtual void OnError( const string& errorText ) = 0;
	virtual void OnCriticalError( const string& errorText ) = 0;
};

class CParser {
public:
	void Parse( const CTokens& tokens );
};

void CParser::Parse( const CTokens& tokens )
{
	tokens;
}

static const char* TokenTypesText[] = {
	"Regex",
	"Number",
	"Identifier",
	"Dot",
	"Comma",
	"DollarSign",
	"NumberSign",
	"VerticalBar",
	"OpeningBrace",
	"ClosingBrace",
	"OpeningBracket",
	"ClosingBracket",
	"OpeningParenthesis",
	"ClosingParenthesis",
	"EqualSign",
	"DoubleEqualSign",
	"Tilde",
	"TildeGreaterThanSign",
	"LessThanSign",
	"DoubleLessThanSign",
	"GreaterThanSign",
	"DoubleGreaterThanSign",
	"ExclamationPointEqualSign"
};

string MakeOffsetUtf8( const string& line, size_t bytesOffset )
{
	string offset;
	if( bytesOffset > line.length() ) {
		bytesOffset = line.length();
	}
	for( size_t i = 0; i < bytesOffset; i++ ) {
		unsigned char c =  static_cast<unsigned char>( line[i] );
		if( c == '\t' ) {
			offset += '\t';
		} else if( ( c & 0xC0 ) != 0x80 ) {
			offset += ' ';
		}
	}
	return offset;
}

size_t LengthUtf8( const string& text )
{
	size_t length = 0;
	for( string::const_iterator c = text.cbegin(); c != text.cend(); ++c ) {
		if( ( static_cast<unsigned char>( *c ) & 0xC0 ) != 0x80 ) {
			length++;
		}
	}
	return length;
}

void Show( const string& line, const CTokens& tokens )
{
	for( auto ti = tokens.cbegin(); ti != tokens.cend(); ++ti ) {
		const CToken& token = *ti;
		cout << line << endl;
		cout << MakeOffsetUtf8( line, token.Offset() );
		cout << string( LengthUtf8(
			line.substr( token.Offset(), token.Length() ) ), '~' ) << endl;
		cout << TokenTypesText[token.Type()];
		switch( token.Type() ) {
			case TT_Regex:
				cout << "   " << '"' << token.Text() << '"';
				break;
			case TT_Identifier:
				cout << " " << token.Text();
				break;
			case TT_Number:
				cout << " " << token.Number() << " " << token.Text();
				break;
			default:
				break;
		}
		cout << endl << endl;
	}
}

void Read( istream& input )
{
	CTokenizer tokenizer;
	CParser parser;
	string line;
	while( input.good() ) {
		getline( input, line, '\n' );
		if( line.empty() ) {
			continue;
		}
		if( line.back() == '\r' ) { // support windows EOL style "\r\n"
			line.pop_back();
		}
		if( line.empty() ) {
			continue;
		}
		tokenizer.TokenizeLine( line, 0 /* line */ );
		if( line.back() == '\\' ) { // remove '\newline' like in C/C++
			continue;
		}
		if( tokenizer.Good() ) {
			Show( line, tokenizer );
			parser.Parse( tokenizer );
		}
		tokenizer.clear();
	}
}

///////////////////////////////////////////////////////////////////////////////

} // end of Lspl namespace

using namespace LsplParser;

int main( int argc, const char* argv[] )
{
	if( argc != 2 ) {
		cerr << "Usage: LSPL_parser FILE" << endl;
		return 1;
	}
	ifstream file( argv[1] );
	if( file.bad() ) {
		cerr << "Error: bad file `" << argv[1] << "`" << endl;
		return 1;
	}
	Read( file );
	return 0;
}
