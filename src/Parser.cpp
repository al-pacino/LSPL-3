#pragma once

#include <common.h>
#include <Parser.h>
#include <Tokenizer.h>
#include <ErrorProcessor.h>

namespace LsplParser {

///////////////////////////////////////////////////////////////////////////////

CPatternParser::CPatternParser( CErrorProcessor& _errorProcessor ) :
	errorProcessor( _errorProcessor )
{
}

void CPatternParser::Parse( const CTokens& _tokens )
{
	tokens = CTokensList( _tokens );

	if( !readPattern() ) {
		return;
	}

	if( !readTextExtractionPatterns() ) {
		return;
	}

	if( tokens.Has() ) {
		addError( "end of template definition expected" );
	}
}

void CPatternParser::addError( const string& text )
{
	CError error( text );

	if( tokens.Has() ) {
		error.Line = tokens->Line;
		error.LineSegments.push_back( tokens.Token() );
	} else {
		error.Line = tokens.Last().Line;
		error.LineSegments.emplace_back();
	}

	errorProcessor.AddError( move( error ) );
}

// reads Identifier [ . Identifier ]
bool CPatternParser::readWordOrPatternName( CWordOrPatternName& name )
{
	name.Reset();

	if( !tokens.CheckType( TT_Identifier ) ) {
		addError( "word class or pattern name expected" );
		return false;
	}

	string nameWithOptionalIndex = tokens->Text;
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
	tokens.Next(); // skip identifier

	if( tokens.MatchType( TT_Dot ) ) {
		if( !tokens.CheckType( TT_Identifier ) ) {
			addError( "word class attribute name expected" );
			return false;
		}

		name.SetSubName( tokens->Text );
		//checkWordClassAttributeName( name.Name() );
		tokens.Next(); // skip identifier
	}

	return true;
}

bool CPatternParser::readPatternName()
{
	if( !tokens.CheckType( TT_Identifier ) ) {
		addError( "template name expected" );
		return false;
	}

	const string patternName = tokens->Text;
	//checkWordClassOrPatternName( patternName );

	tokens.Next();
	return true;
}

bool CPatternParser::readPatternArguments()
{
	if( tokens.MatchType( TT_OpeningParenthesis ) ) {
		do {
			CWordOrPatternName name;
			if( !readWordOrPatternName( name ) ) {
				return false;
			}

			name.Print( cout ); // TODO: temporary
		} while( tokens.MatchType( TT_Comma ) );

		if( !tokens.MatchType( TT_ClosingParenthesis ) ) {
			addError( "closing parenthesis `)` expected" );
			return false;
		}
	}
	return true;
}

bool CPatternParser::readPattern()
{
	if( !readPatternName() || !readPatternArguments() ) {
		return false;
	}
	if( !tokens.MatchType( TT_EqualSign ) ) {
		addError( "equal sign `=` expected" );
		return false;
	}
	unique_ptr<CBasePatternNode> alternatives;
	if( !readAlternatives( alternatives ) ) {
		return false;
	}

	alternatives->Print( cout ); // TODO: temporary
	return true;
}

bool CPatternParser::readElementCondition()
{
	if( tokens.CheckType( TT_Identifier ) &&
		( tokens.CheckType( 1, TT_EqualSign )
			|| tokens.CheckType( 1, TT_ExclamationPointEqualSign ) ) )
	{
		tokens.Next( 2 );
	}

	do {
		if( tokens.MatchType( TT_Regexp ) ) {
			// todo:
		} else if( tokens.MatchType( TT_Identifier ) ) {
			// todo:
		} else {
			addError( "regular expression or word class attribute value expected" );
			return false;
		}
	} while( tokens.MatchType( TT_VerticalBar ) );

	return true;
}

bool CPatternParser::readElementConditions()
{
	if( tokens.MatchType( TT_LessThanSign ) ) {
		do {
			if( !readElementCondition() ) {
				return false;
			}
		} while( tokens.MatchType( TT_Comma ) );

		if( !tokens.MatchType( TT_GreaterThanSign ) ) {
			addError( "greater than sign `>` expected" );
			return false;
		}
	}
	return true;
}

bool CPatternParser::readElement( unique_ptr<CBasePatternNode>& element )
{
	element = nullptr;

	if( tokens.Has() ) {
		switch( tokens->Type ) {
			case TT_Regexp:
				element.reset( new CRegexpNode( tokens->Text ) );
				tokens.Next();
				break;
			case TT_Identifier:
				element.reset( new CWordNode( tokens->Text ) );
				tokens.Next();
				if( !readElementConditions() ) {
					return false;
				}
				break;
			case TT_OpeningBrace:
			{
				tokens.Next();
				unique_ptr<CBasePatternNode> alternatives;
				if( !readAlternatives( alternatives ) ) {
					return false;
				}
				check_logic( static_cast<bool>( alternatives ) );
				if( !tokens.MatchType( TT_ClosingBrace ) ) {
					addError( "closing brace `}` expected" );
					return false;
				}
				size_t min = 0;
				size_t max = numeric_limits<size_t>::max();
				if( tokens.MatchType( TT_LessThanSign ) ) { // < NUMBER, NUMBER >
					if( !tokens.MatchNumber( min ) ) {
						addError( "number (0, 1, 2, etc.) expected" );
						return false;
					}
					if( tokens.MatchType( TT_Comma ) && !tokens.MatchNumber( max ) ) {
						addError( "number (0, 1, 2, etc.) expected" );
						return false;
					}
					if( !tokens.MatchType( TT_GreaterThanSign ) ) {
						addError( "greater than sign `>` expected" );
						return false;
					}
					if( min > max || max == 0 ) {
						addError( "incorrect min max values for repeating" );
					}
				}
				element.reset( new CRepeatingNode( move( alternatives ), min, max ) );
				break;
			}
			case TT_OpeningBracket:
			{
				tokens.Next();
				unique_ptr<CBasePatternNode> alternatives;
				if( !readAlternatives( alternatives ) ) {
					return false;
				}
				check_logic( static_cast<bool>( alternatives ) );
				if( !tokens.MatchType( TT_ClosingBracket ) ) {
					addError( "closing bracket `]` expected" );
					return false;
				}
				element.reset( new CRepeatingNode( move( alternatives ) ) );
			}
			case TT_OpeningParenthesis:
			{
				tokens.Next();
				unique_ptr<CBasePatternNode> alternatives( new CAlternativesNode );
				if( !readAlternatives( alternatives ) ) {
					return false;
				}
				check_logic( static_cast<bool>( alternatives ) );
				if( !tokens.MatchType( TT_ClosingParenthesis ) ) {
					addError( "closing parenthesis `)` expected" );
					return false;
				}
				swap( element, alternatives );
			}
			default:
				break;
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

		name.Print( cout ); // temporary

		if( tokens.MatchType( TT_EqualSign ) ) {
			isEqual = true;
		} else if( tokens.MatchType( TT_DoubleEqualSign ) ) {
			isDoubleEqual = true;
		} else {
			break;
		}
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
	if( !tokens.MatchType( TT_Identifier ) ) {
		addError( "dictionary name expected" );
		return false;
	}
	if( !tokens.MatchType( TT_OpeningParenthesis ) ) {
		addError( "opening parenthesis `(` expected" );
		return false;
	}
	do {
		vector<string> names;
		while( tokens.CheckType( TT_Identifier ) ) {
			names.push_back( tokens->Text );
			tokens.Next();
		}
		if( names.empty() ) {
			addError( "at least one template element expected" );
			return false;
		}
	} while( tokens.MatchType( TT_Comma ) );

	if( !tokens.MatchType( TT_ClosingParenthesis ) ) {
		addError( "closing parenthesis `)` expected" );
		return false;
	}
	return true;
}

bool CPatternParser::readAlternativeCondition()
{
	if( tokens.CheckType( 1, TT_OpeningParenthesis ) ) {
		return readDictionaryCondition();
	} else {
		return readMatchingCondition();
	}
}

// reads << ... >>
bool CPatternParser::readAlternativeConditions()
{
	if( tokens.MatchType( TT_DoubleLessThanSign ) ) {
		do {
			if( !readAlternativeCondition() ) {
				return false;
			}
		} while( tokens.MatchType( TT_Comma ) );

		if( !tokens.MatchType( TT_DoubleGreaterThanSign ) ) {
			addError( "double greater than sign `>>` expected" );
			return false;
		}
	}
	return true;
}

bool CPatternParser::readTransposition( unique_ptr<CBasePatternNode>& out )
{
	out = nullptr;
	unique_ptr<CTranspositionNode> transposition( new CTranspositionNode );
	do {
		unique_ptr<CBasePatternNode> elements;
		if( !readElements( elements ) ) {
			return false;
		}
		check_logic( static_cast<bool>( elements ) );
		transposition->push_back( move( elements ) );
	} while( tokens.MatchType( TT_Tilde ) );
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
	do {
		unique_ptr<CBasePatternNode> transposition;
		if( !readTransposition( transposition ) ) {
			return false;
		}
		check_logic( static_cast<bool>( transposition ) );
		alternatives->push_back( move( transposition ) );
	} while( tokens.MatchType( TT_VerticalBar ) );
	check_logic( !alternatives->empty() );

	if( alternatives->size() == 1 ) {
		swap( out, alternatives->front() );
	} else {
		out.reset( alternatives.release() );
	}

	return true;
}

bool CPatternParser::readTextExtractionPrefix()
{
	CTokensList tmp( tokens );
	if( tmp.MatchType( TT_EqualSign )
		&& tmp.CheckType( TT_Identifier ) && tmp->Text == "text"
		&& tmp.Next() && tmp.CheckType( TT_GreaterThanSign ) )
	{
		tokens.Next( 3 );
		return true;
	}
	return false;
}

bool CPatternParser::readTextExtractionPatterns()
{
	if( readTextExtractionPrefix() ) {
		do {
			if( !readTextExtractionPattern() ) {
				return false;
			}
		} while( tokens.MatchType( TT_Comma ) );
	}
	return true;
}

bool CPatternParser::readTextExtractionPattern()
{
	if( !readTextExtractionElements() ) {
		return false;
	}

	if( tokens.MatchType( TT_DoubleLessThanSign ) ) {
		do {
			CWordOrPatternName name;
			if( !readWordOrPatternName( name ) ) {
				return false;
			}
			if( !tokens.MatchType( TT_TildeGreaterThanSign ) ) {
				addError( "tilde and greater than sign `~>` expected" );
				return false;
			}
			if( !readWordOrPatternName( name ) ) {
				return false;
			}
		} while( tokens.MatchType( TT_Comma ) );

		if( !tokens.MatchType( TT_DoubleGreaterThanSign ) ) {
			addError( "double greater than sign `>>` expected" );
			return false;
		}
	}
	return true;
}

bool CPatternParser::readTextExtractionElements()
{
	if( !readTextExtractionElement( true /* required */ ) ) {
		return false;
	}

	while( readTextExtractionElement() ) {
	}

	return true;
}

bool CPatternParser::readTextExtractionElement( const bool required )
{
	if( tokens.CheckType( TT_Regexp ) ) {
		tokens.Next();
	} else if( tokens.MatchType( TT_NumberSign ) ) {
		if( !tokens.MatchType( TT_Identifier ) ) {
			addError( "word class or pattern name expected" );
			return false;
		}
	} else if( tokens.MatchType( TT_Identifier ) ) {
		if( tokens.MatchType( TT_LessThanSign ) ) {
			while( tokens.MatchType( TT_Identifier ) ) {
				if( tokens.MatchType( TT_Regexp ) ) {
					// todo:
				} else if( tokens.MatchType( TT_Identifier ) ) {
					// todo:
				} else {
					addError( "regular expression or word class attribute value expected" );
					return false;
				}
			}
			if( !tokens.MatchType( TT_GreaterThanSign ) ) {
				addError( "greater than sign `>` expected" );
				return false;
			}
		}
	} else {
		if( required ) {
			addError( "text extraction element expected" );
		}
		return false;
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////

} // end of LsplParser namespace
