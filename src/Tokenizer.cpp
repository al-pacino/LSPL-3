#include <common.h>
#include <Tokenizer.h>
#include <ErrorProcessor.h>
#include <Tools.h>

namespace Lspl {
namespace Parser {

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

inline bool IsIdentifierCharacter( char c )
{
	return !IsByteAsciiSymbol( c )
		|| ( isalnum( c, locale::classic() ) || c == '-' || c == '_' );
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

void CTokenizer::TokenizeLine( CSharedFileLine _line )
{
	initialize( _line );
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
	step( ' ' );
	if( state == &CTokenizer::regexState ) {
		addToken( TT_Regexp, true /* decreaseAnOffsetByOne */ );
		errorProcessor.AddError( CError(
			CLineSegment( offset - text.length(), numeric_limits<size_t>::max() ),
			line, "newline in regular expression" ) );
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
	push_back( CTokenPtr( new CToken( type, line, lineSegment ) ) );
	CToken& token = *back();
	switch( type ) {
		case TT_Regexp:
			token.Text = text;
			token.Length = text.length() + 2;
			break;
		case TT_Number:
			token.Number = stoul( text );
			token.Length = text.length();
			break;
		case TT_Identifier:
			token.Text = text;
			token.Length = text.length();
			break;
		case TT_DoubleEqualSign:
		case TT_TildeGreaterThanSign:
		case TT_DoubleLessThanSign:
		case TT_DoubleGreaterThanSign:
		case TT_ExclamationPointEqualSign:
			token.Length = 2;
			break;
		default:
			break;
	}
	text.clear();
}

void CTokenizer::checkIdentifier()
{
	// check text
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
			if( isdigit( c, locale::classic() ) ) {
				state = &CTokenizer::numberState;
				text.assign( 1, c );
			} else if( IsIdentifierCharacter( c ) ) {
				state = &CTokenizer::indentifierState;
				text.assign( 1, c );
			} else {
				errorProcessor.AddError( CError( CLineSegment( offset ), line,
					"unknown character " + string( 1, c ), ES_CriticalError ) );
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
	if( c == '"' ) {
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
	state = &CTokenizer::regexState;
	text.append( 1, c );
}

void CTokenizer::numberState( char c )
{
	if( isdigit( c, locale::classic() ) ) {
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
		checkIdentifier();
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

void CTokenizer::exclamationSignState( char c )
{
	if( c != '=' ) {
		errorProcessor.AddError( CError( CLineSegment( offset - 1, 2 ),
			line, "incorrect operation, you may possibly mean !=" ) );
	}

	addToken( TT_ExclamationPointEqualSign, true/*decreaseAnOffsetByOne*/ );
	state = &CTokenizer::initialState;
}

///////////////////////////////////////////////////////////////////////////////

} // end of Parser namespace
} // end of Lspl namespace
