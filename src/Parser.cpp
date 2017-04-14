#include <common.h>
#include <Parser.h>
#include <ErrorProcessor.h>
#include <TranspositionSupport.h>
#include <PatternsFileProcessor.h>

namespace Lspl {
namespace Parser {

///////////////////////////////////////////////////////////////////////////////

void CPatternsBuilder::Read( const char* filename )
{
	check_logic( !ErrorProcessor.HasAnyErrors() );

	CPatternsFileProcessor reader( ErrorProcessor, filename );
	CPatternParser parser( ErrorProcessor );

	CTokens tokens;
	while( reader.IsOpen() && !ErrorProcessor.HasCriticalErrors() ) {
		reader.ReadPattern( tokens );
		CPatternDefinitionPtr patternDef = parser.Parse( tokens );
		if( static_cast<bool>( patternDef ) ) {
			if( IsPatternReference( patternDef->Name ) ) {
				auto pair = Names.insert( make_pair(
					CIndexedName( patternDef->Name ).Name,
					PatternDefs.size() ) );

				if( pair.second ) {
					PatternDefs.emplace_back( move( patternDef ) );
				} else {
					ErrorProcessor.AddError(
						CError( *patternDef->Name, "redefinition of pattern" ) );
				}
			} else {
				ErrorProcessor.AddError( CError( *patternDef->Name,
					"pattern name cannot be equal to predefined word" ) );
			}
		}
	}
}

void CPatternsBuilder::Check()
{
	if( ErrorProcessor.HasAnyErrors() ) {
		return;
	}

	for( const CPatternDefinitionPtr& patternDef : PatternDefs ) {
		patternDef->Check( *this );
	}
}

Pattern::CPatterns&& CPatternsBuilder::Save()
{
	check_logic( !ErrorProcessor.HasAnyErrors() );
	return move( *this );
}

void CPatternsBuilder::AddComplexError( const vector<CTokenPtr>& tokens,
	const char* message ) const
{
	debug_check_logic( !tokens.empty() );
	debug_check_logic( static_cast<bool>( tokens.front() ) );

	CError error( tokens.front()->Line, message );
	bool afterSeparator = true;
	for( const CTokenPtr& token : tokens ) {
		if( static_cast<bool>( token ) ) {
			if( token->Line != error.Line ) {
				break;
			}
			if( afterSeparator ) {
				error.LineSegments.push_back( *token );
			} else {
				error.LineSegments.back().Merge( *token );
			}
			afterSeparator = false;
		} else {
			afterSeparator = true;
		}
	}
	ErrorProcessor.AddError( move( error ) );
}

bool CPatternsBuilder::HasElement( const CTokenPtr& elementToken ) const
{
	return ( Elements.find(
		CIndexedName( elementToken ).Normalize() ) != Elements.cend() );
}

bool CPatternsBuilder::CheckSubName( const CTokenPtr& subNameToken,
	const bool patternReference, size_t& index ) const
{
	debug_check_logic( static_cast<bool>( subNameToken ) );

	CIndexedName subName;
	bool found = false;
	if( !subName.Parse( subNameToken ) || patternReference ) {
		found = Configuration().WordSigns().Find( subName.Name, index );
		if( found
			&& Configuration().WordSigns()[index].Type == Configuration::WST_Main )
		{
			ErrorProcessor.AddError( CError( *subNameToken,
				"main word sign is not allowed" ) );
		}
	}
	if( !found ) {
		ErrorProcessor.AddError( CError( *subNameToken,
			"there is no such word sign in configuration" ) );
	}
	return found;
}

bool CPatternsBuilder::CheckSubName( const CTokenPtr& subNameToken,
	const bool patternReference, string& name ) const
{
	debug_check_logic( static_cast<bool>( subNameToken ) );

	CIndexedName subName;
	bool found = false;
	if( !subName.Parse( subNameToken ) || patternReference ) {
		name = subName.Name;
		size_t index;
		found = Configuration().WordSigns().Find( name, index );
		if( found
			&& Configuration().WordSigns()[index].Type == Configuration::WST_Main )
		{
			ErrorProcessor.AddError( CError( *subNameToken,
				"main word sign is not allowed" ) );
		}
		if( !found && patternReference ) {
			found = Configuration().WordSigns().MainWordSign().Values.Has( name );
		}
	}
	if( !found ) {
		name.clear();
		ErrorProcessor.AddError( CError( *subNameToken,
			"there is no such word sign in configuration" ) );
	}
	return found;
}

string CPatternsBuilder::CheckExtendedName(
	const CExtendedName& extendedName ) const
{
	debug_check_logic( static_cast<bool>( extendedName.first ) );
	if( static_cast<bool>( extendedName.second ) ) {
		string name;
		if( CheckSubName( extendedName.second,
				IsPatternReference( extendedName.first ), name ) )
		{
			return name;
		}
	}
	return "";
}

void CPatternsBuilder::CheckPatternExists( const CTokenPtr& reference ) const
{
	if( Names.find( CIndexedName( reference ).Name ) == Names.end() ) {
		ErrorProcessor.AddError( CError( *reference, "undefined pattern" ) );
	}
}

bool CPatternsBuilder::IsPatternReference( const CTokenPtr& reference ) const
{
	return !Configuration().WordSigns().MainWordSign().Values.Has(
		CIndexedName( reference ).Name );
}

///////////////////////////////////////////////////////////////////////////////

void CMatchingCondition::Check( CPatternsBuilder& context ) const
{
	debug_check_logic( Elements.size() >= 2 );
	auto element = Elements.cbegin();
	const string subName = context.CheckExtendedName( *element );
	bool wellFormed = true;
	for( ++element; element != Elements.cend(); ++element ) {
		context.ConditionElements.push_back( element->first );

		const string elementSubName = context.CheckExtendedName( *element );
		if( wellFormed && elementSubName != subName ) {
			wellFormed = false;
		}
	}

	if( !wellFormed ) {
		vector<CTokenPtr> tokens;
		for( const CExtendedName& extendedName : Elements ) {
			tokens.push_back( extendedName.first );
			tokens.push_back( extendedName.second );
			tokens.push_back( nullptr );
		}
		context.AddComplexError( tokens, "inconsistent condition" );
	}
}

void CDictionaryCondition::Check( CPatternsBuilder& context ) const
{
	// STUB:
	context.ErrorProcessor.AddError( CError( *DictionaryName,
		"dictionary conditions are not implemented yet" ) );

	// TODO: check dictionary name
	// TODO: check number of arguments
	for( const auto& argument : Arguments ) {
		for( const CTokenPtr& token : argument ) {
			if( context.IsPatternReference( token ) ) {
				context.ErrorProcessor.AddError( CError( *token,
					"patterns is not allowed in dictionary conditions" ) );
			}
		}
	}
}

void CTranspositionNode::Check( CPatternsBuilder& context ) const
{
	if( this->size() > CTranspositionSupport::MaxTranspositionSize ) {
		context.ErrorProcessor.AddError(
			CError( "transposition cannot contain more than "
			+ to_string( CTranspositionSupport::MaxTranspositionSize )
			+ " elements" ) );
	}
	CPatternNodesSequence<>::Check( context );
}

void CAlternativeNode::Check( CPatternsBuilder& context ) const
{
	node->Check( context );

	for( const unique_ptr<CAlternativeCondition>& condition : conditions ) {
		condition->Check( context );
	}
}

void CRepeatingNode::Check( CPatternsBuilder& context ) const
{
	if( minToken != nullptr && maxToken != nullptr
		&& maxToken->Number < minToken->Number )
	{
		context.AddComplexError( { minToken, nullptr, maxToken },
			"inconsistent min/max repeating value" );
	}

	node->Check( context );
}

void CRegexpNode::Check( CPatternsBuilder& context ) const
{
	// TODO: check regex syntax!
}

void CElementCondition::Check( CPatternsBuilder& context,
	const bool patternReference ) const
{
	const Configuration::CWordSigns& wordSigns = context.Configuration().WordSigns();
	debug_check_logic( !Values.empty() );
	size_t index;
	if( static_cast<bool>( Name ) ) {
		if( !context.CheckSubName( Name, patternReference, index ) ) {
			return;
		}
	} else {
		vector<CTokenPtr> tokens;
		if( static_cast<bool>( EqualSign ) ) {
			tokens.push_back( EqualSign );
		}
		tokens.insert( tokens.end(), Values.cbegin(), Values.cend() );
		context.AddComplexError( tokens,
			"there is no default word sign in configuration" );
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

void CElementNode::Check( CPatternsBuilder& context ) const
{
	CIndexedName name( element );
	context.Elements.insert( name.Normalize() );
	const bool patternReference = !context.Configuration().WordSigns()
		.MainWordSign().Values.Has( name.Name );
	if( patternReference ) {
		context.CheckPatternExists( element );
	}
	for( const CElementCondition& cond : conditions ) {
		cond.Check( context, patternReference );
	}
}

void CPatternDefinition::Check( CPatternsBuilder& context ) const
{
	if( CIndexedName().Parse( Name ) ) {
		context.ErrorProcessor.AddError(
			CError( *Name, "pattern name CANNOT ends with index" ) );
	}

	context.Elements.clear();
	context.ConditionElements.clear();

	Alternatives->Check( context );

	// check ConditionElements
	for( const CTokenPtr& conditionElement : context.ConditionElements ) {
		if( !context.HasElement( conditionElement ) ) {
			context.ErrorProcessor.AddError( CError( *conditionElement,
				"there is no such word in pattern definition" ) );
		}
	}

	// check Arguments
	for( const CExtendedName& extendedName : Arguments ) {
		if( !context.HasElement( extendedName.first ) ) {
			context.ErrorProcessor.AddError( CError( *extendedName.first,
				"there is no such word in pattern definition" ) );
		}
		context.CheckExtendedName( extendedName );
	}
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
