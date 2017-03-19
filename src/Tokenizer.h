#pragma once

#include <SharedFileLine.h>

namespace Lspl {
namespace Parser {

///////////////////////////////////////////////////////////////////////////////

enum TTokenType {
	TT_Regexp, // '"' regualar-expression '"'
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

struct CToken : public CLineSegment {
	TTokenType Type;
	CSharedFileLine Line; // line in the source file
	string Text; // only for identifier and regex
	size_t Number; // only for number

	CToken( TTokenType type, const CSharedFileLine& line,
			const CLineSegment& lineSegment ) :
		CLineSegment( lineSegment ),
		Type( type ),
		Line( line ),
		Number( 0 )
	{
	}

	void Print( ostream& out ) const;
};

typedef shared_ptr<CToken> CTokenPtr;

///////////////////////////////////////////////////////////////////////////////

class CTokens : public vector<CTokenPtr> {
public:
	void Print( ostream& out ) const
	{
		for( const CTokenPtr& token : *this ) {
			token->Print( out );
			out << " ";
		}
	}
};

///////////////////////////////////////////////////////////////////////////////

class CTokensList {
public:
	CTokensList()
	{
	}

	explicit CTokensList( const CTokens& tokens ) :
		token( tokens.cbegin() ),
		end( tokens.cend() )
	{
	}

	const CToken& Last() const
	{
		return *( ( end - 1 )->get() );
	}
	bool Has() const
	{
		return ( token != end );
	}
	bool Next( size_t count = 1 )
	{
		check_logic( count <= static_cast<size_t>( end - token ) );
		token += count;
		return Has();
	}
	const CToken& Token( size_t offset = 0 ) const
	{
		check_logic( offset < static_cast<size_t>( end - token ) );
		return *( ( token + offset )->get() );
	}
	CTokenPtr TokenPtr( size_t offset = 0 ) const
	{
		check_logic( offset < static_cast<size_t>( end - token ) );
		return *( token + offset );
	}
	const CToken* operator->() const
	{
		check_logic( Has() );
		return token->get();
	}
	// checks token exists and its type is tokenType
	bool CheckType( const TTokenType type, size_t offset = 0) const
	{
		if( static_cast<size_t>( end - token ) > offset ) {
			return ( type == Token( offset ).Type );
		}
		return false;
	}
	// if CheckType( type ) does Next
	bool MatchType( const TTokenType type )
	{
		if( CheckType( type ) ) {
			Next();
			return true;
		}
		return false;
	}
	bool MatchType( const TTokenType type, CTokenPtr& tokenPtr )
	{
		if( CheckType( type ) ) {
			tokenPtr = TokenPtr();
			Next();
			return true;
		}
		return false;
	}

private:
	CTokens::const_iterator token;
	CTokens::const_iterator end;
};

///////////////////////////////////////////////////////////////////////////////

class CErrorProcessor;

class CTokenizer : public CTokens {
	CTokenizer( const CTokenizer& ) = delete;
	CTokenizer& operator=( const CTokenizer& ) = delete;

public:
	explicit CTokenizer( CErrorProcessor& errorProcessor );
	~CTokenizer();

	void Reset();
	void TokenizeLine( CSharedFileLine line );

private:
	typedef void ( CTokenizer::*TState )( char c );

	CErrorProcessor& errorProcessor;
	TState state;
	CSharedFileLine line;
	size_t offset; // offset in bytes in the line
	string text; // only for identifier and regex

	void initialize( CSharedFileLine _line );
	void step( char c );
	void finalize();
	void reset();
	void addToken( TTokenType type, bool decreaseAnOffsetByOne = false );
	void checkIdentifier();

	void initialState( char c );
	void commentState( char c );
	void regexState( char c );
	void regexStateAfterBackslash( char c );
	void numberState( char c );
	void indentifierState( char c );
	void tildeState( char c );
	void equalSignState( char c );
	void lessThanSignState( char c );
	void greaterThanSignState( char c );
	void exclamationSignState( char c );
};

///////////////////////////////////////////////////////////////////////////////

} // end of Parser namespace
} // end of Lspl namespace
