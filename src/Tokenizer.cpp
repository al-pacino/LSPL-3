#include <common.h>
#include <Tokenizer.h>
#include <ErrorProcessor.h>

namespace LsplParser {

///////////////////////////////////////////////////////////////////////////////

// mask for ASCII digits
static bitset<256> NumberCharacters( "11111111110000000000000000"
	"00000000000000000000000000000000" );

// mask for ASCII digits, letters, '-' and '_'
static bitset<256> IdentifierCharacters( "11111111111111111111111111"
	"01000011111111111111111111111111" "000000011111111110010000000000000"
	"00000000000000000000000000000000" );

// mask for ASCII control characters
static bitset<256> ControlCharacters( "10000000000000000000000000000000"
	"00000000000000000000000000000000" "00000000000000000000000000000000"
	"11111111111111111111111111111111" );

inline bool IsNumberCharacter( char c )
{
	return NumberCharacters.test( static_cast<unsigned char>( c ) );
}

inline bool IsIdentifierCharacter( char c )
{
	return IdentifierCharacters.test( static_cast<unsigned char>( c ) );
}

inline bool IsControlCharacter( char c )
{
	return ControlCharacters.test( static_cast<unsigned char>( c ) );
}

///////////////////////////////////////////////////////////////////////////////

void CToken::Print( ostream& out ) const
{
	switch( Type ) {
		case TT_Regexp:
			out << '"' << Text << '"';
			break;
		case TT_Number:
			out << Number;
			break;
		case TT_Identifier:
			out << Text;
			break;
		case TT_Dot:
			out << ".";
			break;
		case TT_Comma:
			out << ",";
			break;
		case TT_DollarSign:
			out << "$";
			break;
		case TT_NumberSign:
			out << "#";
			break;
		case TT_VerticalBar:
			out << "|";
			break;
		case TT_OpeningBrace:
			out << "{";
			break;
		case TT_ClosingBrace:
			out << "}";
			break;
		case TT_OpeningBracket:
			out << "[";
			break;
		case TT_ClosingBracket:
			out << "]";
			break;
		case TT_OpeningParenthesis:
			out << "(";
			break;
		case TT_ClosingParenthesis:
			out << ")";
			break;
		case TT_EqualSign:
			out << "=";
			break;
		case TT_DoubleEqualSign:
			out << "==";
			break;
		case TT_Tilde:
			out << "~";
			break;
		case TT_TildeGreaterThanSign:
			out << "~>";
			break;
		case TT_LessThanSign:
			out << "<";
			break;
		case TT_DoubleLessThanSign:
			out << "<<";
			break;
		case TT_GreaterThanSign:
			out << ">";
			break;
		case TT_DoubleGreaterThanSign:
			out << ">>";
			break;
		case TT_ExclamationPointEqualSign:
			out << "!=";
			break;
	}
}

///////////////////////////////////////////////////////////////////////////////

CTokenizer::CTokenizer( CErrorProcessor& _errorProcessor ) :
	errorProcessor( _errorProcessor )
{
	Reset();
}

CTokenizer::~CTokenizer()
{
	Reset();
}

void CTokenizer::Reset()
{
	clear();
	reset();
}

void CTokenizer::TokenizeLine( CSharedFileLine line )
{
	initialize( line );
	for( char c : line->Line ) {
		step( c );
		offset++;
	}
	finalize();
}

void CTokenizer::initialize( CSharedFileLine _line )
{
	check_logic( _line );
	state = &CTokenizer::initialState;
	line = _line;
	text.clear();
	offset = 0;
}

void CTokenizer::step( char c )
{
	( this->*state )( c );
}

void CTokenizer::finalize()
{
	if( state == &CTokenizer::errorState
		|| state == &CTokenizer::commentState )
	{
		return;
	}

	step( ' ' );
	if( state == &CTokenizer::errorState ) {
		return;
	}

	if( state != &CTokenizer::initialState ) {
		check_logic( state == &CTokenizer::regexState );
		error( TE_NewlineInRegex );
	}

	reset();
}

void CTokenizer::reset()
{
	state = nullptr;
	line = CSharedFileLine();
	text.clear();
	offset = 0;
}

void CTokenizer::addToken( TTokenType type, bool decreaseAnOffsetByOne )
{
	CLineSegment lineSegment( offset - text.length() );
	if( decreaseAnOffsetByOne ) {
		lineSegment.Offset--;
	}
	emplace_back( type, line, lineSegment );
	switch( type ) {
		case TT_Regexp:
			back().Text = text;
			back().Length = text.length() + 2;
			break;
		case TT_Number:
			back().Number = stoul( text );
			back().Length = text.length();
			break;
		case TT_Identifier:
			back().Text = text;
			back().Length = text.length();
			break;
		case TT_DoubleEqualSign:
		case TT_TildeGreaterThanSign:
		case TT_DoubleLessThanSign:
		case TT_DoubleGreaterThanSign:
		case TT_ExclamationPointEqualSign:
			back().Length = 2;
			break;
		default:
			break;
	}
	text.clear();
}

void CTokenizer::error( TErrorType errorType )
{
	CError error( line, "", ES_CriticalError );
	switch( errorType ) {
		case TE_UnknowCharacter:
			error.Message = "unknown character";
			error.LineSegments.emplace_back( offset );
			break;
		case TE_NewlineInRegex:
			check_logic( state == &CTokenizer::regexState );
			error.Message = "newline in regular expression";
			error.LineSegments.emplace_back( offset - text.length(),
				numeric_limits<size_t>::max() );
			break;
	}
	errorProcessor.AddError( move( error ) );

	state = &CTokenizer::errorState;
}

void CTokenizer::initialState( char c )
{
	switch( c ) {
		case ' ':
			break; // skip blank character
		case ';':
			state = &CTokenizer::commentState;
			break;
		case '.':
			addToken( TT_Dot );
			break;
		case ',':
			addToken( TT_Comma );
			break;
		case '$':
			addToken( TT_DollarSign );
			break;
		case '#':
			addToken( TT_NumberSign );
			break;
		case '|':
			addToken( TT_VerticalBar );
			break;
		case '{':
			addToken( TT_OpeningBrace );
			break;
		case '}':
			addToken( TT_ClosingBrace );
			break;
		case '[':
			addToken( TT_OpeningBracket );
			break;
		case ']':
			addToken( TT_ClosingBracket );
			break;
		case '(':
			addToken( TT_OpeningParenthesis );
			break;
		case ')':
			addToken( TT_ClosingParenthesis );
			break;
		case '=':
			state = &CTokenizer::equalSignState;
			break;
		case '~':
			state = &CTokenizer::tildeState;
			break;
		case '<':
			state = &CTokenizer::lessThanSignState;
			break;
		case '>':
			state = &CTokenizer::greaterThanSignState;
			break;
		case '!':
			state = &CTokenizer::exclamationSignState;
			break;
		case '"':
			state = &CTokenizer::regexState;
			text.clear();
			break;
		default:
			if( IsNumberCharacter( c ) ) {
				state = &CTokenizer::numberState;
				text.assign( 1, c );
			} else if( IsIdentifierCharacter( c ) ) {
				state = &CTokenizer::indentifierState;
				text.assign( 1, c );
			} else {
				error( TE_UnknowCharacter );
			}		
			break;
	}
}

void CTokenizer::commentState( char /* c */ )
{
	// just skip any characters after ';'
}

void CTokenizer::regexState( char c )
{
	if( IsControlCharacter( c ) ) {
		error( TE_UnknowCharacter );
	} else if( c == '"' ) { 
		addToken( TT_Regexp, true /* decreaseAnOffsetByOne */ );
		state = &CTokenizer::initialState;
	} else {
		if( c == '\\' ) {
			state = &CTokenizer::regexStateAfterBackslash;
		}
		text.append( 1, c );
	}
}

void CTokenizer::regexStateAfterBackslash( char c )
{
	if( IsControlCharacter( c ) ) {
		error( TE_UnknowCharacter );
	} else {
		text.append( 1, c );
		state = &CTokenizer::regexState;
	}
}

void CTokenizer::numberState( char c )
{
	if( IsNumberCharacter( c ) ) {
		text.append( 1, c );
	} else {
		addToken( TT_Number );
		state = &CTokenizer::initialState;
		step( c );
	}
}

void CTokenizer::indentifierState( char c )
{
	if( IsIdentifierCharacter( c ) ) {
		text.append( 1, c );
	} else {
		addToken( TT_Identifier );
		state = &CTokenizer::initialState;
		step( c );
	}
}

void CTokenizer::tildeState( char c )
{
	state = &CTokenizer::initialState;
	if( c == '>' ) {
		addToken( TT_TildeGreaterThanSign, true /* decreaseAnOffsetByOne */ );
	} else {
		addToken( TT_Tilde, true /* decreaseAnOffsetByOne */ );
		step( c );
	}
}

void CTokenizer::equalSignState( char c )
{
	state = &CTokenizer::initialState;
	if( c == '=' ) {
		addToken( TT_DoubleEqualSign, true /* decreaseAnOffsetByOne */ );
	} else {
		addToken( TT_EqualSign, true /* decreaseAnOffsetByOne */ );
		step( c );
	}
}

void CTokenizer::lessThanSignState( char c )
{
	state = &CTokenizer::initialState;
	if( c == '<' ) {
		addToken( TT_DoubleLessThanSign, true /* decreaseAnOffsetByOne */ );
	} else {
		addToken( TT_LessThanSign, true /* decreaseAnOffsetByOne */ );
		step( c );
	}
}

void CTokenizer::greaterThanSignState( char c )
{
	state = &CTokenizer::initialState;
	if( c == '>' ) {
		addToken( TT_DoubleGreaterThanSign, true/* decreaseAnOffsetByOne */ );
	} else {
		addToken( TT_GreaterThanSign, true /* decreaseAnOffsetByOne */ );
		step( c );
	}
}

void CTokenizer::errorState( char /* c */ )
{
	// just skip rest characters in line
}

void CTokenizer::exclamationSignState( char c )
{
	if( c == '=' ) {
		addToken( TT_ExclamationPointEqualSign, true/*decreaseAnOffsetByOne*/ );
		state = &CTokenizer::initialState;
	} else {
		offset--; // dirty hack to show error in exclamation point
		error( TE_UnknowCharacter );
		offset++; // restore the correct value
	}
}

///////////////////////////////////////////////////////////////////////////////

} // end of Lspl namespace
