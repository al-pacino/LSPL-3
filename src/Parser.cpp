#include <common.h>
#include <Parser.h>
#include <Tokenizer.h>
#include <ErrorProcessor.h>

namespace Lspl {
namespace Parser {

///////////////////////////////////////////////////////////////////////////////

void AddTokensError( CErrorProcessor& errorProcessor,
	const vector<CTokenPtr>& tokens, const char* message,
	const bool merge = false )
{
	debug_check_logic( !tokens.empty() );
	debug_check_logic( static_cast<bool>( tokens.front() ) );

	CError error( *tokens.front(), message );
	for( const CTokenPtr& token : tokens ) {
		debug_check_logic( static_cast<bool>( token ) );
		if( token->Line != error.Line ) {
			break;
		}
		error.LineSegments.push_back( *token );
	}
	if( merge ) {
		const size_t begin = error.LineSegments.front().Offset;
		const size_t end = error.LineSegments.back().EndOffset();
		error.LineSegments.clear();
		error.LineSegments.emplace_back( begin, end - begin );
	}

	errorProcessor.AddError( move( error ) );
}

///////////////////////////////////////////////////////////////////////////////

void CMatchingCondition::Check( CPatternDefinitionCheckContext& context ) const
{
	// TODO:
}

void CDictionaryCondition::Check( CPatternDefinitionCheckContext& context ) const
{
	// TODO: check dictionary name
	// TODO: check number of arguments

	const Configuration::COrderedStrings& mainValues =
		context.Configuration.WordSigns().MainWordSign().Values;

	for( const auto& argument : Arguments ) {
		for( const CTokenPtr& token : argument ) {
			CIndexedName name( token );
			if( !mainValues.Has( name.Name ) ) {
				context.ErrorProcessor.AddError( CError( *token,
					"pattern is not allowed in dictionary conditions" ) );
			}
		}
	}
}

void CAlternativeNode::Check( CPatternDefinitionCheckContext& context ) const
{
	node->Check( context );

	for( const unique_ptr<CAlternativeCondition>& condition : conditions ) {
		condition->Check( context );
	}
}

void CRepeatingNode::Check( CPatternDefinitionCheckContext& context ) const
{
	if( min != nullptr && max != nullptr && max->Number < min->Number ) {
		AddTokensError( context.ErrorProcessor, { min, max },
			"inconsistent min/max repeating value" );
	}

	node->Check( context );
}

void CRegexpNode::Check( CPatternDefinitionCheckContext& context ) const
{
	// TODO: check regex syntax!
}

void CElementCondition::Check( CPatternDefinitionCheckContext& context,
	const bool patternReference ) const
{
	const Configuration::CWordSigns& wordSigns = context.Configuration.WordSigns();
	debug_check_logic( !Values.empty() );
	size_t index;
	if( static_cast<bool>( Name ) ) {
		CIndexedName name;
		bool found = false;
		if( !name.Parse( Name ) || patternReference ) {
			found = wordSigns.Find( name.Name, index );
			if( found && !patternReference
				&& wordSigns[index].Type == Configuration::WST_Main )
			{
				context.ErrorProcessor.AddError( CError( *Name,
					"main word sign is not allowed for predefined words" ) );
				return;
			}
		}
		if( !found ) {
			context.ErrorProcessor.AddError( CError( *Name,
				"there is no such word sign in configuration" ) );
			return;
		}
	} else {
		vector<CTokenPtr> tokens;
		if( static_cast<bool>( EqualSign ) ) {
			tokens.push_back( EqualSign );
		}
		tokens.insert( tokens.end(), Values.cbegin(), Values.cend() );
		AddTokensError( context.ErrorProcessor, tokens,
			"there is no default word sign in configuration", true /* merge */ );
		return;
	}

	const Configuration::CWordSign& wordSign = wordSigns[index];
	/*if( wordSign.Type == Configuration::WST_String ) {
		for( const CTokenPtr& tokenPtr : Values ) {
			if( tokenPtr->Type != TT_Regexp ) {
				context.ErrorProcessor.AddError( CError( *tokenPtr,
					"string constant expected" ) );
			}
		}
	} else {*/
	for( const CTokenPtr& value : Values ) {
		debug_check_logic( value->Type == TT_Identifier || value->Type == TT_Regexp );
		if( !wordSign.Values.Has( value->Text ) ) {
			context.ErrorProcessor.AddError( CError( *value,
				"there is no such word sign value for this word sign in configuration" ) );
		}
	}
}

void CElementNode::Check( CPatternDefinitionCheckContext& context ) const
{
	CIndexedName name( element );
	context.Elements.insert( name.Normalize() );
	const bool patternReference = !context.Configuration.WordSigns()
		.MainWordSign().Values.Has( name.Name );
	if( patternReference ) {
		context.PatternReferences.push_back( element );
	}
	for( const CElementCondition& cond : conditions ) {
		cond.Check( context, patternReference );
	}
}

string CPatternDefinition::Check(
	const Configuration::CConfiguration& configuration,
	CErrorProcessor& errorProcessor,
	vector<CTokenPtr>& references ) const
{
	CIndexedName IndexedName;
	if( IndexedName.Parse( Name ) ) {
		errorProcessor.AddError( CError( *Name,
			"pattern name CANNOT ends with index" ) );
	}

	const Configuration::COrderedStrings& mainValues
		= configuration.WordSigns().MainWordSign().Values;

	if( mainValues.Has( IndexedName.Name ) ) {
		errorProcessor.AddError( CError( *Name,
			"pattern name CANNOT be equal to predefined word" ) );
		IndexedName.Name.clear();
	}

	CPatternDefinitionCheckContext context( configuration, errorProcessor, references );
	Alternatives->Check( context );

	// check Arguments
	for( const CExtendedName& extendedName : Arguments ) {
		CIndexedName name( extendedName.first );
		if( context.Elements.find( name.Normalize() ) == context.Elements.end() ) {
			errorProcessor.AddError( CError( *(extendedName.first),
				"there is no such word in pattern definition" ) );
		}

		const bool patternReference = !mainValues.Has( name.Name );
		if( static_cast<bool>( extendedName.second ) ) {
			CIndexedName subName;
			bool found = false;
			if( !subName.Parse( extendedName.second ) || patternReference ) {
				size_t index;
				found = configuration.WordSigns().Find( subName.Name, index );
				if( found && !patternReference && configuration.WordSigns()[index].Type
						== Configuration::WST_Main )
				{
					errorProcessor.AddError( CError( *( extendedName.second ),
						"main word sign is not allowed for predefined words" ) );
				}
			}
			if( !found ) {
				errorProcessor.AddError( CError( *( extendedName.second ),
					"there is no such word sign in configuration" ) );
			}
		}
	}

	return IndexedName.Name;
}

///////////////////////////////////////////////////////////////////////////////

CPatternParser::CPatternParser( CErrorProcessor& _errorProcessor ) :
	errorProcessor( _errorProcessor )
{
}

CPatternDefinitionPtr CPatternParser::Parse( const CTokens& _tokens )
{
	tokens = CTokensList( _tokens );

	CPatternDefinitionPtr patternDef( new CPatternDefinition );
	if( !readPattern( *patternDef ) ) {
		return CPatternDefinitionPtr();
	}

	if( !readTextExtractionPatterns() ) {
		return CPatternDefinitionPtr();
	}

	if( tokens.Has() ) {
		addError( "end of template definition expected" );
		return CPatternDefinitionPtr();
	}

	return patternDef;
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
bool CPatternParser::readExtendedName( CExtendedName& name )
{
	if( !tokens.CheckType( TT_Identifier ) ) {
		addError( "word class or pattern name expected" );
		return false;
	}

	name.first = tokens.TokenPtr();
	tokens.Next(); // skip identifier

	if( tokens.MatchType( TT_Dot ) ) {
		if( !tokens.CheckType( TT_Identifier ) ) {
			addError( "word class attribute name expected" );
			return false;
		}

		name.second = tokens.TokenPtr();
		tokens.Next(); // skip identifier
	} else {
		name.second = nullptr;
	}

	return true;
}

bool CPatternParser::readPatternName( CPatternDefinition& patternDef )
{
	debug_check_logic( !static_cast<bool>( patternDef.Name ) );

	if( !tokens.CheckType( TT_Identifier ) ) {
		addError( "pattern name expected" );
		return false;
	}

	patternDef.Name = tokens.TokenPtr();
	tokens.Next();
	return true;
}

bool CPatternParser::readPatternArguments( CPatternDefinition& patternDef )
{
	debug_check_logic( patternDef.Arguments.empty() );

	if( tokens.MatchType( TT_OpeningParenthesis ) ) {
		CExtendedNames& arguments = patternDef.Arguments;
		do {
			arguments.emplace_back();
			if( !readExtendedName( arguments.back() ) ) {
				return false;
			}
		} while( tokens.MatchType( TT_Comma ) );

		if( !tokens.MatchType( TT_ClosingParenthesis ) ) {
			addError( "closing parenthesis `)` expected" );
			return false;
		}
	}
	return true;
}

bool CPatternParser::readPattern( CPatternDefinition& patternDef )
{
	if( !readPatternName( patternDef ) ) {
		return false;
	}
	if( !readPatternArguments( patternDef ) ) {
		return false;
	}
	if( !tokens.MatchType( TT_EqualSign ) ) {
		addError( "equal sign `=` expected" );
		return false;
	}

	debug_check_logic( !static_cast<bool>( patternDef.Alternatives ) );
	if( !readAlternatives( patternDef.Alternatives ) ) {
		return false;
	}

	return true;
}

bool CPatternParser::readElementCondition( CElementCondition& elementCondition )
{
	elementCondition.Clear();

	if( tokens.CheckType( TT_Identifier ) &&
		( tokens.CheckType( TT_EqualSign, 1 /* offset */ )
			|| tokens.CheckType( TT_ExclamationPointEqualSign, 1 /* offset */ ) ) )
	{
		elementCondition.Name = tokens.TokenPtr();
		elementCondition.EqualSign = tokens.TokenPtr( 1 /* offset */ );
		tokens.Next( 2 );
	} else if( tokens.CheckType( TT_EqualSign )
		|| tokens.CheckType( TT_ExclamationPointEqualSign ) )
	{
		elementCondition.EqualSign = tokens.TokenPtr();
		tokens.Next();
	}

	do {
		if( tokens.CheckType( TT_Regexp ) || tokens.CheckType( TT_Identifier ) ) {
			elementCondition.Values.push_back( tokens.TokenPtr() );
			tokens.Next();
		} else {
			addError( "regular expression or word class attribute value expected" );
			return false;
		}
	} while( tokens.MatchType( TT_VerticalBar ) );

	return true;
}

bool CPatternParser::readElementConditions( CElementConditions& elementConditions )
{
	elementConditions.clear();

	if( tokens.MatchType( TT_LessThanSign ) ) {
		do {
			elementConditions.emplace_back();
			if( !readElementCondition( elementConditions.back() ) ) {
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
				element.reset( new CRegexpNode( tokens.TokenPtr() ) );
				tokens.Next();
				break;
			case TT_Identifier:
			{
				unique_ptr<CElementNode> tmpElement( new CElementNode( tokens.TokenPtr() ) );
				tokens.Next();
				if( !readElementConditions( tmpElement->Conditions() ) ) {
					return false;
				}
				element = move( tmpElement );
				break;
			}
			case TT_OpeningBrace:
			{
				tokens.Next();
				unique_ptr<CAlternativesNode> alternatives;
				if( !readAlternatives( alternatives ) ) {
					return false;
				}
				check_logic( static_cast<bool>( alternatives ) );
				if( !tokens.MatchType( TT_ClosingBrace ) ) {
					addError( "closing brace `}` expected" );
					return false;
				}

				CTokenPtr min;
				CTokenPtr max;
				if( tokens.MatchType( TT_LessThanSign ) ) { // < NUMBER, NUMBER >
					if( !tokens.MatchType( TT_Number, min ) ) {
						addError( "number (0, 1, 2, etc.) expected" );
						return false;
					}
					if( tokens.MatchType( TT_Comma )
						&& !tokens.MatchType( TT_Number, max ) )
					{
						addError( "number (0, 1, 2, etc.) expected" );
						return false;
					}
					if( !tokens.MatchType( TT_GreaterThanSign ) ) {
						addError( "greater than sign `>` expected" );
						return false;
					}
				}
				element.reset( new CRepeatingNode( move( alternatives ), min, max ) );
				break;
			}
			case TT_OpeningBracket:
			{
				tokens.Next();
				unique_ptr<CAlternativesNode> alternatives;
				if( !readAlternatives( alternatives ) ) {
					return false;
				}
				check_logic( static_cast<bool>( alternatives ) );
				if( !tokens.MatchType( TT_ClosingBracket ) ) {
					addError( "closing bracket `]` expected" );
					return false;
				}
				element.reset( new CRepeatingNode( move( alternatives ) ) );
				break;
			}
			case TT_OpeningParenthesis:
			{
				tokens.Next();
				unique_ptr<CAlternativesNode> alternatives;
				if( !readAlternatives( alternatives ) ) {
					return false;
				}
				check_logic( static_cast<bool>( alternatives ) );
				if( !tokens.MatchType( TT_ClosingParenthesis ) ) {
					addError( "closing parenthesis `)` expected" );
					return false;
				}
				element = move( alternatives );
				break;
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

bool CPatternParser::readMatchingCondition( CMatchingCondition& condition )
{
	condition.Elements.emplace_back();
	if( !readExtendedName( condition.Elements.back() ) ) {
		return false;
	}

	condition.Strong = tokens.MatchType( TT_DoubleEqualSign );
	if( !condition.Strong && !tokens.MatchType( TT_EqualSign ) ) {
		addError( "equal sign `=` or double equal `==` sign expected" );
		return false;
	}

	do {
		condition.Elements.emplace_back();
		if( !readExtendedName( condition.Elements.back() ) ) {
			return false;
		}

		if( tokens.CheckType( TT_EqualSign ) ) {
			if( condition.Strong ) {
				addError( "inconsistent equal sign `=` and double equal `==` sign" );
			}
		} else if( tokens.CheckType( TT_DoubleEqualSign ) ) {
			if( !condition.Strong ) {
				addError( "inconsistent equal sign `=` and double equal `==` sign" );
			}
		}
	} while( tokens.MatchType( TT_EqualSign ) || tokens.MatchType( TT_DoubleEqualSign ) );

	return true;
}

// reads TT_Identifier `(` TT_Identifier { TT_Identifier } { `,` TT_Identifier { TT_Identifier } } `)`
bool CPatternParser::readDictionaryCondition( CDictionaryCondition& condition )
{
	if( !tokens.MatchType( TT_Identifier, condition.DictionaryName ) ) {
		addError( "dictionary name expected" );
		return false;
	}
	if( !tokens.MatchType( TT_OpeningParenthesis ) ) {
		addError( "opening parenthesis `(` expected" );
		return false;
	}
	do {
		condition.Arguments.emplace_back();
		while( tokens.CheckType( TT_Identifier ) ) {
			condition.Arguments.back().push_back( tokens.TokenPtr() );
			tokens.Next();
		}
		if( condition.Arguments.back().empty() ) {
			addError( "at least one pattern element expected" );
			return false;
		}
	} while( tokens.MatchType( TT_Comma ) );

	if( !tokens.MatchType( TT_ClosingParenthesis ) ) {
		addError( "closing parenthesis `)` expected" );
		return false;
	}
	return true;
}

bool CPatternParser::readAlternativeCondition( CAlternativeConditions& conditions )
{
	if( tokens.CheckType( TT_OpeningParenthesis, 1 /* offset */ ) ) {
		unique_ptr<CDictionaryCondition> condition( new CDictionaryCondition );
		if( !readDictionaryCondition( *condition ) ) {
			return false;
		}
		conditions.emplace_back( condition.release() );
	} else {
		unique_ptr<CMatchingCondition> condition( new CMatchingCondition );
		if( !readMatchingCondition( *condition ) ) {
			return false;
		}
		conditions.emplace_back( condition.release() );
	}
	return true;
}

// reads << ... >>
bool CPatternParser::readAlternativeConditions( CAlternativeConditions& conditions )
{
	if( tokens.MatchType( TT_DoubleLessThanSign ) ) {
		do {
			if( !readAlternativeCondition( conditions ) ) {
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

bool CPatternParser::readAlternative( unique_ptr<CAlternativeNode>& alternative )
{
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

	if( transposition->size() == 1 ) {
		alternative.reset( new CAlternativeNode( move( transposition->front() ) ) );
	} else {
		alternative.reset( new CAlternativeNode( move( transposition ) ) );
	}

	if( !readAlternativeConditions( alternative->Conditions() ) ) {
		return false;
	}

	return true;
}

bool CPatternParser::readAlternatives( unique_ptr<CAlternativesNode>& alternatives )
{
	alternatives.reset( new CAlternativesNode );

	do {
		unique_ptr<CAlternativeNode> alternative;
		if( !readAlternative( alternative ) ) {
			return false;
		}
		check_logic( static_cast<bool>( alternative ) );
		alternatives->push_back( move( alternative ) );
	} while( tokens.MatchType( TT_VerticalBar ) );
	check_logic( !alternatives->empty() );

	return true;
}

bool CPatternParser::readTextExtractionPrefix()
{
	if( tokens.CheckType( TT_EqualSign )
		&& tokens.CheckType( TT_Identifier, 1 /* offset */ )
		&& tokens.Token( 1 /* offset */ ).Text == "text"
		&& tokens.CheckType( TT_GreaterThanSign, 2 /* offset */ ) )
	{
		tokens.Next( 3 /* count */ );
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
			CExtendedName name;
			if( !readExtendedName( name ) ) {
				return false;
			}
			if( !tokens.MatchType( TT_TildeGreaterThanSign ) ) {
				addError( "tilde and greater than sign `~>` expected" );
				return false;
			}
			if( !readExtendedName( name ) ) {
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

} // end of Parser namespace
} // end of Lspl namespace
