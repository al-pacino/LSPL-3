#pragma once

#include <common.h>
#include <Parser.h>
#include <Tokenizer.h>
#include <ErrorProcessor.h>

namespace LsplParser {

///////////////////////////////////////////////////////////////////////////////

bool CPatternParser::Parse()
{
	return readPattern();
}

void CPatternParser::addError( const string& text )
{
	CError error( text );

	if( hasToken() ) {
		error.Line = token->Line;
		error.LineSegments.push_back( *token );
	} else {
		error.Line = tokens.back().Line;
		error.LineSegments.emplace_back();
	}
}

// checks that current token exists and its type is tokenType
bool CPatternParser::isTokenOfType( TTokenType tokenType ) const
{
	return ( hasToken() && token->Type == tokenType );
}

// shifts to the next token and does isTokenOfType
bool CPatternParser::isNextTokenOfType( TTokenType tokenType )
{
	return ( nextToken() && token->Type == tokenType );
}

// reads Identifier [ . Identifier ]
bool CPatternParser::readWordOrPatternName( CWordOrPatternName& name )
{
	name.Reset();

	if( !isTokenOfType( TT_Identifier ) ) {
		addError( "word class or template name expected" );
		return false;
	}

	string nameWithOptionalIndex = token->Text;
	const size_t pos = nameWithOptionalIndex.find_last_not_of( "0123456789" );
	check_logic( pos != string::npos );
	name.SetName( nameWithOptionalIndex.substr( 0, pos + 1 ) );
	const string index = nameWithOptionalIndex.substr( pos + 1 );
	if( !index.empty() ) {
		name.SetIndex( stoul( index ) );
		if( name.Index() == 0 ) {
			addError( "name index must be positive (1, 2, 3, etc.)" );
		}
	}
	//checkWordClassOrPatternName( name.Name() );

	if( isNextTokenOfType( TT_Dot ) ) {
		if( !isNextTokenOfType( TT_Identifier ) ) {
			addError( "word class attribute name expected" );
			return false;
		}

		name.SetSubName( token->Text );
		//checkWordClassAttributeName( name.Name() );
		nextToken();
	}

	return true;
}

bool CPatternParser::readPatternName()
{
	if( !isTokenOfType( TT_Identifier ) ) {
		addError( "template name expected" );
		return false;
	}

	const string patternName = token->Text;
	//checkWordClassOrPatternName( patternName );

	nextToken();
	return true;
}

bool CPatternParser::readPatternArguments()
{
	if( isTokenOfType( TT_OpeningParenthesis ) ) {
		while( true ) {
			nextToken();

			CWordOrPatternName name;
			if( !readWordOrPatternName( name ) ) {
				return false;
			}

			name.Print( cout );

			if( !isTokenOfType( TT_Comma ) ) {
				break;
			}
		}
		if( !isTokenOfType( TT_ClosingParenthesis ) ) {
			addError( "closing parenthesis `)` expected" );
			return false;
		}
		nextToken();
	}
	return true;
}

bool CPatternParser::readPattern()
{
	if( readPatternName() && readPatternArguments() ) {
		if( isTokenOfType( TT_EqualSign ) ) {
			nextToken();
			unique_ptr<CBasePatternNode> alternatives;
			if( readAlternatives( alternatives ) ) {
				if( hasToken() ) {
					addError( "end of template definition expected" );
				} else {
					alternatives->Print( cout );
					return true;
				}
			}
		} else {
			addError( "equal sign `=` expected" );
		}
	}
	return false;
}

bool CPatternParser::readWordCondition()
{
	auto testToken = token;
	if( testToken != tokens.cend()
		&& testToken->Type == TT_Identifier
		&& ++testToken != tokens.cend()
		&& ( testToken->Type == TT_EqualSign
			|| testToken->Type == TT_ExclamationPointEqualSign ) )
	{
		// todo:
		nextToken();
		nextToken();
	}

	while( true ) {
		if( isTokenOfType( TT_Regexp ) ) {
			// todo:
		} else if( isTokenOfType( TT_Identifier ) ) {
			// todo:
		} else {
			addError( "regular expression or word class attribute value expected" );
			return false;
		}
		if( !isNextTokenOfType( TT_VerticalBar ) ) {
			break;
		}
		nextToken();
	}

	return true;
}

bool CPatternParser::readWordConditions()
{
	if( isTokenOfType( TT_LessThanSign ) ) {
		while( true ) {
			nextToken();
			if( !readWordCondition() ) {
				return false;
			}
			if( !isTokenOfType( TT_Comma ) ) {
				break;
			}
		}
		if( !isTokenOfType( TT_GreaterThanSign ) ) {
			addError( "greater than sign `>` expected" );
			return false;
		}
		nextToken();
	}
	return true;
}

bool CPatternParser::readElement( unique_ptr<CBasePatternNode>& element )
{
	element = nullptr;

	if( hasToken() ) {
		if( token->Type == TT_Regexp ) {
			element.reset( new CRegexpNode( token->Text ) );
			nextToken();
		} else if( token->Type == TT_Identifier ) {
			element.reset( new CWordNode( token->Text ) );
			nextToken();
			if( !readWordConditions() ) {
				return false;
			}
		} else if( token->Type == TT_OpeningBrace ) {
			nextToken();
			unique_ptr<CBasePatternNode> alternatives;
			if( !readAlternatives( alternatives ) ) {
				return false;
			}
			check_logic( static_cast<bool>( alternatives ) );
			if( !isTokenOfType( TT_ClosingBrace ) ) {
				addError( "closing brace `}` expected" );
				return false;
			}
			size_t min = 0;
			size_t max = numeric_limits<size_t>::max();
			if( isNextTokenOfType( TT_LessThanSign ) ) { // < NUMBER, NUMBER >
				if( !isNextTokenOfType( TT_Number ) ) {
					addError( "number (0, 1, 2, etc.) expected" );
					return false;
				}
				min = token->Number;
				if( isNextTokenOfType( TT_Comma ) ) {
					if( !isNextTokenOfType( TT_Number ) ) {
						addError( "number (0, 1, 2, etc.) expected" );
						return false;
					}
					max = token->Number;
					nextToken();
				}
				if( !isTokenOfType( TT_GreaterThanSign ) ) {
					addError( "greater than sign `>` expected" );
					return false;
				}
				if( min > max || max == 0 ) {
					addError( "incorrect min max values for repeating" );
				}
				nextToken();
			}
			element.reset( new CRepeatingNode( move( alternatives ), min, max ) );
		} else if( token->Type == TT_OpeningBracket ) {
			nextToken();
			unique_ptr<CBasePatternNode> alternatives;
			if( !readAlternatives( alternatives ) ) {
				return false;
			}
			check_logic( static_cast<bool>( alternatives ) );
			if( !isTokenOfType( TT_ClosingBracket ) ) {
				addError( "closing bracket `]` expected" );
				return false;
			}
			nextToken();
			element.reset( new CRepeatingNode( move( alternatives ) ) );
		} else if( token->Type == TT_OpeningParenthesis ) {
			nextToken();
			unique_ptr<CBasePatternNode> alternatives( new CAlternativesNode );
			if( !readAlternatives( alternatives ) ) {
				return false;
			}
			check_logic( static_cast<bool>( alternatives ) );
			if( !isTokenOfType( TT_ClosingParenthesis ) ) {
				addError( "closing parenthesis `)` expected" );
				return false;
			}
			nextToken();
			swap( element, alternatives );
		}
	}
	return true;
}

bool CPatternParser::readElements( unique_ptr<CBasePatternNode>& out )
{
	out = nullptr;
	unique_ptr<CElementsNode> elements( new CElementsNode );
	while( true ) {
		unique_ptr<CBasePatternNode> element;
		if( !readElement( element ) ) {
			return false;
		}

		if( !element ) {
			break;
		}
		elements->push_back( move( element ) );
	}
	if( elements->empty() ) {
		addError( "at least one template element expected" );
		return false;
	}

	if( elements->size() == 1 ) {
		swap( out, elements->front() );
	} else {
		out.reset( elements.release() );
	}

	return true;
}

bool CPatternParser::readMatchingCondition()
{
	bool isEqual = false;
	bool isDoubleEqual = false;
	while( true ) {
		CWordOrPatternName name;
		if( !readWordOrPatternName( name ) ) {
			return false;
		}
		name.Print( cout );

		if( isTokenOfType( TT_EqualSign ) ) {
			isEqual = true;
		} else if( isTokenOfType( TT_DoubleEqualSign ) ) {
			isDoubleEqual = true;
		} else {
			break;
		}
		nextToken();
	}
	if( !isEqual && !isDoubleEqual ) {
		addError( "equal sign `=` or double equal `==` sign expected" );
		return false;
	}
	if( isEqual && isDoubleEqual ) {
		addError( "inconsistent equal sign `=` and double equal `==` sign" );
	}
	return true;
}

// reads TT_Identifier `(` TT_Identifier { TT_Identifier } { `,` TT_Identifier { TT_Identifier } } `)`
bool CPatternParser::readDictionaryCondition()
{
	if( !isTokenOfType( TT_Identifier ) ) {
		addError( "dictionary name expected" );
		return false;
	}
	if( !isNextTokenOfType( TT_OpeningParenthesis ) ) {
		addError( "opening parenthesis `(` expected" );
		return false;
	}
	while( true ) {
		vector<string> names;
		while( isNextTokenOfType( TT_Identifier ) ) {
			names.push_back( token->Text );
		}
		if( names.empty() ) {
			addError( "at least one template element expected" );
			return false;
		}
		if( !isTokenOfType( TT_Comma ) ) {
			break;
		}
	}
	if( !isTokenOfType( TT_ClosingParenthesis ) ) {
		addError( "closing parenthesis `)` expected" );
		return false;
	}
	nextToken();
	return true;
}

bool CPatternParser::readAlternativeCondition()
{
	auto testToken = token;
	if( testToken != tokens.cend()
		&& ++testToken != tokens.cend()
		&& testToken->Type == TT_OpeningParenthesis )
	{
		return readDictionaryCondition();
	} else {
		return readMatchingCondition();
	}
}

// reads << ... >>
bool CPatternParser::readAlternativeConditions()
{
	if( isTokenOfType( TT_DoubleLessThanSign ) ) {
		while( true ) {
			nextToken();
			if( !readAlternativeCondition() ) {
				return false;
			}
			if( !isTokenOfType( TT_Comma ) ) {
				break;
			}
		}
		if( !isTokenOfType( TT_DoubleGreaterThanSign ) ) {
			addError( "double greater than sign `>>` expected" );
			return false;
		}
		nextToken();
	}
	return true;
}

bool CPatternParser::readTransposition( unique_ptr<CBasePatternNode>& out )
{
	out = nullptr;
	unique_ptr<CTranspositionNode> transposition( new CTranspositionNode );
	while( true ) {
		unique_ptr<CBasePatternNode> elements;
		if( !readElements( elements ) ) {
			return false;
		}
		check_logic( static_cast<bool>( elements ) );
		transposition->push_back( move( elements ) );
		if( isTokenOfType( TT_Tilde ) ) {
			nextToken();
		} else {
			break;
		}
	}
	check_logic( !transposition->empty() );

	if( !readAlternativeConditions() ) {
		return false;
	}

	if( transposition->size() == 1 ) {
		swap( out, transposition->front() );
	} else {
		out.reset( transposition.release() );
	}

	return true;
}

bool CPatternParser::readAlternatives( unique_ptr<CBasePatternNode>& out )
{
	out = nullptr;
	unique_ptr<CAlternativesNode> alternatives( new CAlternativesNode );
	while( true ) {
		unique_ptr<CBasePatternNode> transposition;
		if( !readTransposition( transposition ) ) {
			return false;
		}
		check_logic( static_cast<bool>( transposition ) );
		alternatives->push_back( move( transposition ) );
		if( token != tokens.cend() && token->Type == TT_VerticalBar ) {
			++token;
		} else {
			break;
		}
	}
	check_logic( !alternatives->empty() );

	if( alternatives->size() == 1 ) {
		swap( out, alternatives->front() );
	} else {
		out.reset( alternatives.release() );
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////

} // end of LsplParser namespace
