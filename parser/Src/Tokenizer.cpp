#include <Tokenizer.h>

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

size_t CToken::Length() const
{
	switch( Type() ) {
		case TT_Regex:
			return text.length() + 2;
		case TT_Number:
		case TT_Identifier:
			return text.length();
		case TT_Dot:
		case TT_Comma:
		case TT_DollarSign:
		case TT_NumberSign:
		case TT_VerticalBar:
		case TT_OpeningBrace:
		case TT_ClosingBrace:
		case TT_OpeningBracket:
		case TT_ClosingBracket:
		case TT_OpeningParenthesis:
		case TT_ClosingParenthesis:
		case TT_EqualSign:
		case TT_Tilde:
		case TT_LessThanSign:
		case TT_GreaterThanSign:
			return 1;
		case TT_DoubleEqualSign:
		case TT_TildeGreaterThanSign:
		case TT_DoubleLessThanSign:
		case TT_DoubleGreaterThanSign:
		case TT_ExclamationPointEqualSign:
			return 2;
	}
	assert( false );
	return 0;
}

///////////////////////////////////////////////////////////////////////////////

CTokenizer::CTokenizer():
	good( true )
{
}

void CTokenizer::TokenizeLine( const string& str, size_t _line )
{
	state = &CTokenizer::initialState;
	text.clear();
	offset = 0;
	line = _line;
	for( string::const_iterator c = str.cbegin(); c != str.cend(); ++c ) {
		step( *c );
		offset++;
	}
	if( state != &CTokenizer::errorState ) {
		finalize();
	}
}

void CTokenizer::initialState( char c )
{
	switch( c ) {
		case ' ': case '\t':
			break; // skip blank character
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

void CTokenizer::regexState( char c )
{
	if( IsControlCharacter( c ) ) {
		error( TE_UnknowCharacter );
	} else if( c == '"' ) { 
		addToken( TT_Regex, true /* decreaseAnOffsetByOne */ );
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
		size_t number = stoul( text );
		addToken( TT_Number );
		back().number = number;
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

void CTokenizer::finalize()
{
	assert( state != &CTokenizer::errorState );
	step( ' ' );
	if( state != &CTokenizer::initialState ) {
		assert( state == &CTokenizer::regexState );
		error( TE_NewlineInRegex );
	}
}

void CTokenizer::exclamationSignState( char c )
{
	if( c == '=' ) {
		addToken( TT_ExclamationPointEqualSign, true/*decreaseAnOffsetByOne*/ );
		state = &CTokenizer::initialState;
	} else {
		error( TE_UnknowCharacter ); // todo: think about it
	}
}

void CTokenizer::addToken( TTokenType type, bool decreaseAnOffsetByOne )
{
	push_back( CToken( type, line,
		offset - text.length() - ( decreaseAnOffsetByOne ? 1 : 0 ), text ) );
	text.clear();
}

void CTokenizer::error( TTokenizationError error )
{
	state = &CTokenizer::errorState;
	good = false;
	// todo: add error processing
	cout << "error: " << offset << ", " << static_cast<int>( error ) << endl;
}

///////////////////////////////////////////////////////////////////////////////

} // end of Lspl namespace
