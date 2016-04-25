#pragma once

#include <CommonStd.h>

namespace LsplParser {

///////////////////////////////////////////////////////////////////////////////

enum TTokenType {
	TT_Regex, // '"' regualar-expression '"'
	TT_Number, // a sequence of digits
	TT_Identifier, // a sequence of digits, latin letters, '-' and '_'
	TT_Dot, // '.'
	TT_Comma, // ','
	TT_DollarSign, // '$'
	TT_NumberSign, // '#'
	TT_VerticalBar, // '|'
	TT_OpeningBrace, // '{'
	TT_ClosingBrace, // '}'
	TT_OpeningBracket, // '['
	TT_ClosingBracket, // ']'
	TT_OpeningParenthesis, // '('
	TT_ClosingParenthesis, // ')'
	TT_EqualSign, // '='
	TT_DoubleEqualSign, // '=='
	TT_Tilde, // '~'
	TT_TildeGreaterThanSign, // '~>'
	TT_LessThanSign, // '<'
	TT_DoubleLessThanSign, // '<<'
	TT_GreaterThanSign, // '>'
	TT_DoubleGreaterThanSign, // '>>'
	TT_ExclamationPointEqualSign // '!='
};

///////////////////////////////////////////////////////////////////////////////

class CToken {
	friend class CTokenizer;

public:
	TTokenType Type() const { return type; }
	size_t Line() const { return line; }
	size_t Offset() const { return offset; }
	const string& Text() const;
	size_t Number() const;
	// calculates the length in bytes
	size_t Length() const;

private:
	CToken( TTokenType _type, size_t _line = 0, size_t _offset = 0,
			const string& _text = "" ):
		type( _type ),
		line( _line ),
		offset( _offset ),
		text( _text ),
		number( 0 )
	{
	}

	TTokenType type;
	size_t line; // line in source file
	size_t offset; // offset in bytes in line of source file
	string text; // only for identifier, regex and number
	size_t number; // only for number
};

inline const string& CToken::Text() const
{
	assert( Type() == TT_Identifier || Type() == TT_Regex || Type() == TT_Number );
	return text;
}

inline size_t CToken::Number() const
{
	assert( Type() == TT_Number );
	return number;
}

typedef vector<CToken> CTokens;

///////////////////////////////////////////////////////////////////////////////
// possible errors

enum TTokenizationError {
	TE_UnknowCharacter, // unknown character 'hexnumber', like C2018
	TE_NewlineInRegex // newline in regular expression, like C2001
};

///////////////////////////////////////////////////////////////////////////////

class CTokenizer : public CTokens {
public:
	CTokenizer();

	void TokenizeLine( const string& line );
	bool Good() const { return good; }

private:
	CTokenizer( const CTokenizer& );
	CTokenizer& operator=( const CTokenizer& );

	typedef void ( CTokenizer::*TState )( char c );

	TState state;
	size_t offset; // offset in bytes in line of source file
	string text; // only for identifier and regex
	bool good;

	void initialState( char c );
	void regexState( char c );
	void regexStateAfterBackslash( char c );
	void numberState( char c );
	void indentifierState( char c );
	void tildeState( char c );
	void equalSignState( char c );
	void lessThanSignState( char c );
	void greaterThanSignState( char c );
	void exclamationSignState( char c );
	void errorState( char c );

	void step( char c ) { ( this->*state )( c ); }
	void finalize();
	void addToken( TTokenType type, bool decreaseAnOffsetByOne = false );
	void error( TTokenizationError error );
};

///////////////////////////////////////////////////////////////////////////////

} // end of LsplParser namespace
