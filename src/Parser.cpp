#include <common.h>
#include <Parser.h>
#include <ErrorProcessor.h>
#include <TranspositionSupport.h>
#include <PatternsFileProcessor.h>

using namespace Lspl::Pattern;
using namespace Lspl::Configuration;

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

	Patterns.reserve( PatternDefs.size() );
	for( const CPatternDefinitionPtr& patternDef : PatternDefs ) {
		CPatternBasePtr pattern = patternDef->Check( *this );
		CPattern* rawPattern = dynamic_cast<CPattern*>( pattern.get() );
		check_logic( rawPattern != nullptr );
		Patterns.emplace_back( move( *rawPattern ) );
	}
}

Pattern::CPatterns CPatternsBuilder::Save()
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

CPatternArgument CPatternsBuilder::CheckExtendedName(
	const CExtendedName& extendedName ) const
{
	const CWordSigns& signs = Configuration().WordSigns();
	const CWordSign& main = signs.MainWordSign();

	const CIndexedName name( extendedName.first );
	size_t index = 0;
	const bool patternReference = !main.Values.Find( name.Name, index );

	if( !patternReference ) {
		index += name.Index * main.Values.Size(); // correct element index
		if( static_cast<bool>( extendedName.second ) ) {
			CIndexedName subName;
			CWordSigns::SizeType subIndex;
			const bool found = !subName.Parse( extendedName.second )
				&& signs.Find( subName.Name, subIndex );

			if( found ) {
				if( signs[subIndex].Type != Configuration::WST_Main ) {
					return CPatternArgument( index, PAT_ElementSign, subIndex );
				} else {
					ErrorProcessor.AddError( CError( *extendedName.second,
						"main word sign is not allowed here" ) );
				}
			} else {
				ErrorProcessor.AddError( CError( *extendedName.second,
					"there is no such word sign in configuration" ) );
			}
		} else {
			return CPatternArgument( index );
		}
	} else if( static_cast<bool>( extendedName.second ) ) {
		auto pi = Names.find( name.Name );
		if( pi != Names.cend() ) {
			const CIndexedName subName( extendedName.second );
			CPatternArgument arg
				= PatternDefs[pi->second]->Argument( subName.Index, *this );

			if( main.Values.Find( subName.Name, index ) ) {
				index += subName.Index * main.Values.Size(); // correct element index
				if( arg.Type == PAT_ReferenceElement && arg.Element == index ) {
					return arg;
				}
			} else {
				CWordSigns::SizeType subIndex;
				if( signs.Find( subName.Name, subIndex )
					&& signs[subIndex].Type != Configuration::WST_Main )
				{
					if( arg.Type == PAT_ReferenceElementSign
						&& arg.Sign == subIndex )
					{
						return arg;
					} else if( arg.Type == PAT_ReferenceElement ) {
						arg.Type = PAT_ReferenceElementSign;
						arg.Sign = subIndex;
						return arg;
					}
				}
			}
		}
		ErrorProcessor.AddError( CError( *extendedName.second,
			"pattern argument was not found" ) );
	} else {
		ErrorProcessor.AddError( CError( *extendedName.first,
			"omitted pattern argument" ) );
	}
	return CPatternArgument();
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

CPatternBasePtr CPatternsBuilder::BuildElement( const CTokenPtr& reference,
	CSignRestrictions&& signRestrictions ) const
{
	const CWordSign& main = Configuration().WordSigns().MainWordSign();

	CIndexedName name( reference );
	CWordSigns::SizeType index;
	if( main.Values.Find( name.Name, index ) ) {
		index += name.Index * main.Values.Size();
		return CPatternBasePtr(
			new CPatternElement( index, move( signRestrictions ) ) );
	} else {
		return CPatternBasePtr( new CPatternReference(
			GetReference( reference ), move( signRestrictions ) ) );
	}
}

CSignValues::ValueType CPatternsBuilder::StringIndex( const string& str )
{
	auto pair = StringIndices.insert( make_pair( str, Strings.size() ) );
	if( pair.second ) {
		Strings.push_back( str );
	}
	return pair.first->second;
}

TReference CPatternsBuilder::GetReference( const CTokenPtr& reference ) const
{
	const CIndexedName name( reference );
	auto pi = Names.find( name.Name );
	if( pi == Names.cend() ) {
		return numeric_limits<CWordSigns::SizeType>::max();
	}
	return ( pi->second + name.Index * Names.size() );
}

///////////////////////////////////////////////////////////////////////////////

void CMatchingCondition::Check( CPatternsBuilder& context,
	Pattern::CPatternConditions& conditions ) const
{
	debug_check_logic( Elements.size() >= 2 );
	auto element = Elements.cbegin();
	const CPatternArgument first = context.CheckExtendedName( *element );
	CPatternArguments arguments( 1, first );
	bool wellFormed = true;
	for( ++element; element != Elements.cend(); ++element ) {
		context.ConditionElements.push_back( element->first );
		const CPatternArgument current = context.CheckExtendedName( *element );
		if( wellFormed && first.Inconsistent( current ) ) {
			wellFormed = false;
		}
		arguments.push_back( current );
	}

	if( wellFormed ) {
		conditions.emplace_back( Strong, move( arguments ) );
	} else {
		vector<CTokenPtr> tokens;
		for( const CExtendedName& extendedName : Elements ) {
			tokens.push_back( extendedName.first );
			tokens.push_back( extendedName.second );
			tokens.push_back( nullptr );
		}
		context.AddComplexError( tokens, "inconsistent condition" );
	}
}

///////////////////////////////////////////////////////////////////////////////

void CDictionaryCondition::Check( CPatternsBuilder& context,
	Pattern::CPatternConditions& /*conditions*/ ) const
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

	// TODO: add dicitionary Condition
}

///////////////////////////////////////////////////////////////////////////////

void CTranspositionNode::Print( ostream& out ) const
{
	PrintAll( out, " ~ " );
}

CPatternBasePtr CTranspositionNode::Check( CPatternsBuilder& context ) const
{
	if( this->size() > CTranspositionSupport::MaxTranspositionSize ) {
		context.ErrorProcessor.AddError(
			CError( "transposition cannot contain more than "
			+ to_string( CTranspositionSupport::MaxTranspositionSize )
			+ " elements" ) );
	}

	return CPatternBasePtr( new CPatternSequence(
		CPatternNodesSequence<>::CheckAll( context ),
		true /* transposition */ ) );
}

///////////////////////////////////////////////////////////////////////////////

void CElementsNode::Print( ostream& out ) const
{
	PrintAll( out, " " );
}

CPatternBasePtr CElementsNode::Check( CPatternsBuilder& context ) const
{
	return CPatternBasePtr( new CPatternSequence(
		CPatternNodesSequence<>::CheckAll( context ) ) );
}

///////////////////////////////////////////////////////////////////////////////

void CAlternativeNode::Print( ostream& out ) const
{
	node->Print( out );
	out << getConditions();
}

CPatternBasePtr CAlternativeNode::Check( CPatternsBuilder& context ) const
{
	CPatternBasePtr element = node->Check( context );

	CPatternConditions patternConditions;
	for( const unique_ptr<CAlternativeCondition>& condition : conditions ) {
		condition->Check( context, patternConditions );
	}

	return CPatternBasePtr(
		new CPatternAlternative( move( element ), move( patternConditions ) ) );
}

///////////////////////////////////////////////////////////////////////////////

void CAlternativesNode::Print( ostream& out ) const
{
	out << "( ";
	PrintAll( out, " | " );
	out << " )";
}

CPatternBasePtr CAlternativesNode::Check( CPatternsBuilder& context ) const
{
	return CPatternBasePtr( new CPatternAlternatives(
		CPatternNodesSequence<CAlternativeNode>::CheckAll( context ) ) );
}

///////////////////////////////////////////////////////////////////////////////

void CRepeatingNode::Print( ostream& out ) const
{
	out << "{ ";
	node->Print( out );
	out << " }";
	if( minToken ) {
		out << "<";
		minToken->Print( out );
		if( maxToken ) {
			out << ",";
			maxToken->Print( out );
		}
		out << ">";
	}
}

CPatternBasePtr CRepeatingNode::Check( CPatternsBuilder& context ) const
{
	if( minToken != nullptr && maxToken != nullptr
		&& maxToken->Number < minToken->Number )
	{
		context.AddComplexError( { minToken, nullptr, maxToken },
			"inconsistent min/max repeating value" );
	}

	size_t mic = getMinCount();
	size_t mac = getMaxCount();
	if( mac < mic ) {
		mic = 0;
		mac = 1;
	}

	return CPatternBasePtr(
		new CPatternRepeating( node->Check( context ), mic, mac ) );
}

///////////////////////////////////////////////////////////////////////////////

void CRegexpNode::Print( ostream& out ) const
{
	regexp->Print( out );
}

CPatternBasePtr CRegexpNode::Check( CPatternsBuilder& context ) const
{
	// TODO: check regex syntax!
	debug_check_logic( regexp->Type == TT_Regexp );
	return CPatternBasePtr( new CPatternRegexp( regexp->Text ) );
}

///////////////////////////////////////////////////////////////////////////////

vector<CTokenPtr> CElementCondition::collectTokens() const
{
	vector<CTokenPtr> tokens;
	if( static_cast<bool>( Name ) ) {
		tokens.push_back( Name );
	}
	if( static_cast<bool>( EqualSign ) ) {
		tokens.push_back( EqualSign );
	}
	tokens.insert( tokens.end(), Values.cbegin(), Values.cend() );
	return tokens;
}

void CElementCondition::Check( CPatternsBuilder& context,
	const CTokenPtr& element, CSignRestrictions& signRestrictions ) const
{
	const CWordSigns& wordSigns = context.Configuration().WordSigns();

	debug_check_logic( !Values.empty() );
	CPatternArgument arg;
	if( static_cast<bool>( Name ) ) {
		arg = context.CheckExtendedName( { element, Name } );
		if( !arg.Defined() ) {
			return;
		}
		if( arg.Sign == wordSigns.MainWordSignIndex() ) {
			context.ErrorProcessor.AddError( CError( *Name,
				"main word sign is not allowed here" ) );
		}
	} else {
		context.AddComplexError( collectTokens(),
			"there is no default word sign in configuration" );
		return;
	}

	CSignValues signValues;
	const CWordSign& wordSign = wordSigns[arg.Sign];
	if( wordSign.Type == Configuration::WST_String ) {
		for( const CTokenPtr& value : Values ) {
			debug_check_logic( value->Type == TT_Identifier || value->Type == TT_Regexp );
			/*if( tokenPtr->Type != TT_Regexp ) {
				context.ErrorProcessor.AddError( CError( *tokenPtr,
					"string constant expected" ) );
			}*/
			signValues.Add( context.StringIndex( value->Text ) );
		}
	} else {
		for( const CTokenPtr& value : Values ) {
			debug_check_logic( value->Type == TT_Identifier || value->Type == TT_Regexp );
			CSignValues::ValueType signValue;
			if( wordSign.Values.Find( value->Text, signValue ) ) {
				if( !signValues.Add( signValue ) ) {
					context.ErrorProcessor.AddError( CError( *value,
						"duplicate word sign value" ) );
				}
			} else {
				context.ErrorProcessor.AddError( CError( *value,
					"there is no such word sign value for the word sign in configuration" ) );
			}
		}
	}

	if( arg.Type != PAT_None && !signValues.IsEmpty() ) {
		const bool exclude = static_cast<bool>( EqualSign )
			&& ( EqualSign->Type == TT_ExclamationPointEqualSign );
		if( !signRestrictions.Add( CSignRestriction( arg.Element, arg.Sign,
											move( signValues ), exclude ) ) )
		{
			context.AddComplexError( collectTokens(),
				"duplicate word sign" );
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

void CElementNode::Print( ostream& out ) const
{
	element->Print( out );
	if( conditions.empty() ) {
		return;
	}
	out << "<";
	bool isFirst = true;
	for( const CElementCondition& cond : conditions ) {
		if( !isFirst ) {
			out << ",";
		}
		isFirst = false;
		if( cond.Name ) {
			cond.Name->Print( out );
		}
		if( cond.EqualSign ) {
			cond.EqualSign->Print( out );
		}
		bool isInternalFirst = true;
		for( CTokenPtr value : cond.Values ) {
			if( !isInternalFirst ) {
				out << "|";
			}
			isInternalFirst = false;
			value->Print( out );
		}
	}
	out << ">";
}

CPatternBasePtr CElementNode::Check( CPatternsBuilder& context ) const
{
	context.Elements.insert( CIndexedName( element ).Normalize() );

	if( context.IsPatternReference( element ) ) {
		context.CheckPatternExists( element );
	}

	CSignRestrictions signRestrictions;
	for( const CElementCondition& cond : conditions ) {
		cond.Check( context, element, signRestrictions );
	}

	return context.BuildElement( element, move( signRestrictions ) );
}

///////////////////////////////////////////////////////////////////////////////

CPatternArgument CPatternDefinition::Argument( const size_t argIndex,
	const CPatternsBuilder& context ) const
{
	if( argIndex < Arguments.size() ) {
		const CExtendedName& extendedName = Arguments[argIndex];

		const CWordSigns& signs = context.Configuration().WordSigns();
		const CWordSign& main = signs.MainWordSign();

		const CIndexedName name( extendedName.first );
		size_t index = 0;
		const bool valid = main.Values.Find( name.Name, index );
		index += argIndex * main.Values.Size(); // correct

		if( valid ) {
			if( static_cast<bool>( extendedName.second ) ) {
				CIndexedName subName;

				CWordSigns::SizeType subIndex;
				if( !subName.Parse( extendedName.second ) // name without index
					&& signs.Find( subName.Name, subIndex ) // sign existst
					&& signs[subIndex].Type != Configuration::WST_Main ) // sign isn't main
				{
					return CPatternArgument( index, PAT_ReferenceElementSign,
						subIndex, context.GetReference( Name ) );
				}
			} else {
				return CPatternArgument( index, PAT_ReferenceElement, 0,
					context.GetReference( Name ) );
			}
		}
	}
	return CPatternArgument();
}

void CPatternDefinition::Print( ostream& out ) const
{
	Name->Print( out );
	out << " =";
	Alternatives->Print( out );
	out << endl;
}

CPatternBasePtr CPatternDefinition::Check( CPatternsBuilder& context ) const
{
	CIndexedName name;
	if( name.Parse( Name ) ) {
		context.ErrorProcessor.AddError(
			CError( *Name, "pattern name CANNOT ends with index" ) );
	}

	context.Elements.clear();
	context.ConditionElements.clear();

	CPatternBasePtr root = Alternatives->Check( context );

	// check ConditionElements
	for( const CTokenPtr& conditionElement : context.ConditionElements ) {
		if( !context.HasElement( conditionElement ) ) {
			context.ErrorProcessor.AddError( CError( *conditionElement,
				"there is no such word in pattern definition" ) );
		}
	}

	// check Arguments
	CPatternArguments patternArguments;
	for( const CExtendedName& extendedName : Arguments ) {
		if( !context.HasElement( extendedName.first ) ) {
			context.ErrorProcessor.AddError( CError( *extendedName.first,
				"there is no such word in pattern definition" ) );
		}
		const CPatternArgument arg = context.CheckExtendedName( extendedName );
		if( arg.HasReference() ) {
			context.ErrorProcessor.AddError( CError( *extendedName.first,
				"pattern cannot be used as argument" ) );
		} else {
			patternArguments.push_back( arg );
		}
	}

	return CPatternBasePtr(
		new CPattern( name.Name, move( root ), patternArguments ) );
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
