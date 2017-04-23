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

CIndexedName::CIndexedName() :
	Index( 0 )
{
}

CIndexedName::CIndexedName( const CTokenPtr& token )
{
	Parse( token );
}

bool CIndexedName::Parse( const CTokenPtr& token )
{
	debug_check_logic( static_cast<bool>( token ) );
	debug_check_logic( token->Type == TT_Identifier );
	Name = token->Text;
	const size_t pos = Name.find_last_not_of( "0123456789" );
	debug_check_logic( pos != string::npos );
	if( pos < Name.length() - 1 ) {
		Index = stoul( Name.substr( pos + 1 ) );
		Name.erase( pos + 1 );
		return true;
	}
	Index = 0;
	return false;
}

string CIndexedName::Normalize() const
{
	return ( Name + to_string( Index ) );
}

///////////////////////////////////////////////////////////////////////////////

class CConditionsCheckContext {
public:
	CPatternsBuilder& Context;

	explicit CConditionsCheckContext( CPatternsBuilder& context ) :
		Context( context )
	{
	}

	void Add( const CExtendedName& name1, const CPatternArgument& word1,
		const CExtendedName& name2, const CPatternArgument& word2,
		const bool strong )
	{
		const CKey key( strong, name1, word1, name2, word2 );
		auto pair = conditions.insert( make_pair( key, conditions.size() ) );
		if( !pair.second ) {
			addAgreementsOverlappedError( pair.first->first.Names, key.Names );
			if( key.Include( pair.first->first ) ) {
				const size_t index = pair.first->second;
				conditions.erase( pair.first );
				pair = conditions.insert( make_pair( key, index ) );
				debug_check_logic( pair.second );
			}
		}
	}

	void Add( const CTokenPtr& dictionary, const CExtendedNames& names,
		CPatternArguments&& words )
	{
		debug_check_logic( static_cast<bool>( dictionary ) );
		CKey key( dictionary->Text, names, move( words ) );
		auto pair = conditions.insert( make_pair( move( key ), conditions.size() ) );
		if( !pair.second ) {
			addAgreementsOverlappedError( { CExtendedName{ dictionary, nullptr } }, names );
		}
	}

	CConditions Build() const
	{
		vector<CMap::const_iterator> order( conditions.size(), conditions.cend() );
		for( CMap::const_iterator i = conditions.cbegin(); i != conditions.cend(); ++i ) {
			debug_check_logic( order[i->second] == conditions.cend() );
			order[i->second] = i;
		}

		vector<CCondition> result;
		for( const CMap::const_iterator condition : order ) {
			debug_check_logic( condition != conditions.end() );
			const CKey& key = condition->first;
			if( key.Dictionary.empty() ) {
				result.emplace_back( key.Strong, key.Words );
			} else {
				result.emplace_back( key.Dictionary, key.Words );
			}
		}

		return CConditions( move( result ) );
	}

private:
	void addAgreementsOverlappedError( const CExtendedNames& names1,
		const CExtendedNames& names2 ) const
	{
		vector<CTokenPtr> tokens;
		for( const CExtendedName& name : names1 ) {
			tokens.push_back( name.first );
			tokens.push_back( name.second );
			tokens.push_back( nullptr );
		}
		for( const CExtendedName& name : names2 ) {
			tokens.push_back( name.first );
			tokens.push_back( name.second );
			tokens.push_back( nullptr );
		}
		Context.AddComplexError( tokens, "agreements overlapped" );
	}

	struct CKey {
		bool Strong;
		string Dictionary;
		CExtendedNames Names;
		CPatternArguments Words;

		CKey( const bool strong,
				const CExtendedName& name1, const CPatternArgument& word1,
				const CExtendedName& name2, const CPatternArgument& word2 ) :
			Strong( strong )
		{
			debug_check_logic( word1.Type != PAT_None );
			debug_check_logic( word2.Type != PAT_None );
			if( ComparatorLess{}( word1, word2 ) ) {
				Names = { name1, name2 };
				Words = { word1, word2 };
			} else {
				Names = { name2, name1 };
				Words = { word2, word1 };
			}
		}

		CKey( const string& dictionary, const CExtendedNames& names,
				CPatternArguments&& words ) :
			Strong( false ),
			Dictionary( dictionary ),
			Names( names ),
			Words( move( words ) )
		{
			debug_check_logic( !dictionary.empty() );
			debug_check_logic( Names.size() == Words.size() );
			debug_check_logic( !Words.empty() );
		}

		struct ComparatorLess {
			bool operator()( const CPatternArgument& arg1,
				const CPatternArgument& arg2 ) const
			{
				if( arg1.Type < arg2.Type ) {
					return true;
				} else if( arg1.Type > arg2.Type ) {
					return false;
				}

				if( arg1.Reference < arg2.Reference ) {
					return true;
				} else if( arg1.Reference > arg2.Reference ) {
					return false;
				}

				if( arg1.Element < arg2.Element ) {
					return true;
				} else if( arg1.Element > arg2.Element ) {
					return false;
				}

				if( arg1.Sign < arg2.Sign ) {
					return true;
				} else if( arg1.Sign > arg2.Sign ) {
					return false;
				}

				return true;
			}
		};

		struct ComparatorIgnoreSign {
			bool operator()( const CPatternArgument& arg1,
				const CPatternArgument& arg2 ) const
			{
				if( arg1.Element != arg2.Element ) {
					return false;
				}
				if( arg1.Type == PAT_ReferenceElement
					|| arg1.Type == PAT_ReferenceElementSign )
				{
					if( arg2.Type == PAT_ReferenceElement
						|| arg2.Type == PAT_ReferenceElementSign )
					{
						return ( arg1.Reference == arg2.Reference );
					}
					return false;
				}
				return true;
			}
		};

		bool Include( const CKey& key ) const
		{
			if( !equal( Words.cbegin(), Words.cend(), key.Words.cbegin(),
					ComparatorIgnoreSign{} ) )
			{
				return false;
			}
			if( Words[0].HasSign() ) {
				if( key.Words[0].HasSign()
					&& key.Words[0].Sign == Words[0].Sign )
				{
					return ( Strong || !key.Strong );
				} else {
					return false;
				}
			} else {
				return ( Strong || !key.Strong );
			}
		}

		struct Hasher {
			size_t operator()( const CKey& key ) const
			{
				size_t hashVal = hash<string>{}( key.Dictionary );
				for( const CPatternArgument& word : key.Words ) {
					hashVal ^= ( word.Reference << 4 ) ^ ( word.Element << 16 );
				}
				return hashVal;
			}
		};
		struct Comparator {
			bool operator()( const CKey& key1, const CKey& key2 ) const
			{
				if( key1.Dictionary != key2.Dictionary ) {
					return false;
				}
				CPatternArgument::Comparator comparator;
				if( !key1.Dictionary.empty() ) {
					return ( key1.Words.size() == key1.Words.size() )
						&& equal( key1.Words.begin(), key1.Words.end(),
							key2.Words.begin(), comparator );
				}
				debug_check_logic( key1.Words.size() == 2 );
				debug_check_logic( key2.Words.size() == 2 );
				return key1.Include( key2 ) || key2.Include( key1 );
			}
		};
	};

	typedef unordered_map<CKey, size_t, CKey::Hasher, CKey::Comparator> CMap;
	CMap conditions;
};

///////////////////////////////////////////////////////////////////////////////

CAlternativeCondition::CAlternativeCondition( CExtendedNames&& _names ) :
	names( _names )
{
	debug_check_logic( !names.empty() );
}

CAlternativeCondition::CAlternativeCondition( CTokenPtr _dictionary,
		CExtendedNames&& _names ) :
	dictionary( _dictionary ),
	names( _names )
{
	debug_check_logic( static_cast<bool>( dictionary ) );
	debug_check_logic( !names.empty() );
}

void CAlternativeCondition::checkAgreement(
	CConditionsCheckContext& context ) const
{
	debug_check_logic( !static_cast<bool>( dictionary ) );

	bool strong;
	CPatternArguments words;
	if( checkConsistency( context.Context, strong, words ) ) {
		for( CPatternArguments::size_type i = 0; i < words.size(); i++ ) {
			for( CPatternArguments::size_type j = i + 1; j < words.size(); j++ ) {
				context.Add( names[i * 2], words[i], names[j * 2], words[j], strong );
				if( !strong ) {
					break;
				}
			}
		}
	}
}

bool CAlternativeCondition::checkConsistency( CPatternsBuilder& context,
	bool& strong, CPatternArguments& words ) const
{
	debug_check_logic( !static_cast<bool>( dictionary ) );
	debug_check_logic( names.size() % 2 == 1 );

	words.emplace_back( context.CheckExtendedName( names[0] ) );
	if( names.size() == 1 ) {
		context.AddComplexError( { names[0].first, names[0].second },
			"agreement condition can not contain only one word" );
		return false;
	}

	strong = isDoubleEqualSign( names[1] );

	bool defined = words.front().Defined();
	bool wordConsistency = true;
	bool equalConsistency = true;
	for( CExtendedNames::size_type i = 2; i < names.size(); i++ ) {
		if( i % 2 == 0 ) {
			// word
			context.ConditionElements.push_back( names[i].first );
			words.emplace_back( context.CheckExtendedName( names[i] ) );
			defined &= words.back().Defined();
			if( words.front().Inconsistent( words.back() ) ) {
				wordConsistency = false;
			}
		} else {
			// = or ==
			if( strong != isDoubleEqualSign( names[i] ) ) {
				equalConsistency = false;
			}
		}
	}

	if( !wordConsistency || !equalConsistency ) {
		vector<CTokenPtr> words;
		vector<CTokenPtr> equals;

		for( CExtendedNames::size_type i = 0; i < names.size(); i++ ) {
			if( i % 2 == 0 ) {
				// word
				words.push_back( names[i].first );
				words.push_back( names[i].second );
				if( static_cast<bool>( words.back() ) ) {
					words.push_back( nullptr );
				}
			} else {
				// = or ==
				equals.push_back( names[i].second );
				equals.push_back( nullptr );
			}
		}

		if( !equalConsistency ) {
			context.AddComplexError( equals,
				"inconsistent comparison in the condition" );
		}

		if( !wordConsistency ) {
			context.AddComplexError( words,
				"inconsistent attributes in the condition" );
		}

		return false;
	}
	return defined;
}

bool CAlternativeCondition::isDoubleEqualSign( const CExtendedName& name ) const
{
	debug_check_logic( !static_cast<bool>( name.first ) );
	debug_check_logic( static_cast<bool>( name.second ) );
	if( name.second->Type == TT_DoubleEqualSign ) {
		return true;
	} else {
		debug_check_logic( name.second->Type == TT_EqualSign );
		return false;
	}
}

void CAlternativeCondition::checkDictionary(
	CConditionsCheckContext& context ) const
{
	debug_check_logic( static_cast<bool>( dictionary ) );
	debug_check_logic( !dictionary->Text.empty() );
	// STUB:
	context.Context.ErrorProcessor.AddError( CError( *dictionary,
		"dictionary conditions are not implemented yet" ) );

	// TODO: check dictionary name
	// TODO: check number of arguments
	CPatternArguments words;
	for( const CExtendedName& name : names ) {
		if( !static_cast<bool>( name.first ) ) {
			debug_check_logic( static_cast<bool>( name.second ) );
			words.emplace_back();
			continue;
		}
		debug_check_logic( !static_cast<bool>( name.second ) );
		if( context.Context.IsPatternReference( name.first ) ) {
			context.Context.ErrorProcessor.AddError( CError( *name.first,
				"pattern is not allowed in dictionary conditions" ) );
		} else {
			context.Context.ConditionElements.push_back( name.first );
			words.push_back( context.Context.CheckExtendedName( name ) );
			debug_check_logic( words.back().Type == PAT_Element );
		}
	}

	if( words.size() == names.size() ) {
		context.Add( dictionary, names, move( words ) );
	}
}

void CAlternativeCondition::Check( CConditionsCheckContext& context ) const
{
	if( static_cast<bool>( dictionary ) ) {
		checkDictionary( context );
	} else {
		checkAgreement( context );
	}
}

void CAlternativeCondition::Print( ostream& out ) const
{
	if( static_cast<bool>( dictionary ) ) {
		dictionary->Print( out );
		out << "(";
	}

	bool first = true;
	for( const CExtendedName& name : names ) {
		if( static_cast<bool>( name.first ) ) {
			if( first ) {
				first = false;
			} else {
				out << " ";
			}
			name.first->Print( out );
			if( static_cast<bool>( name.second ) ) {
				out << ".";
				name.second->Print( out );
			}
		} else {
			debug_check_logic( static_cast<bool>( name.second ) );
			name.second->Print( out );
			first = true;
		}
	}

	if( static_cast<bool>( dictionary ) ) {
		out << ")";
	}
}

///////////////////////////////////////////////////////////////////////////////

CConditions CAlternativeConditions::Check( CPatternsBuilder& context ) const
{
	CConditionsCheckContext conditionsCheckContext( context );
	for( const CAlternativeCondition& condition : *this ) {
		condition.Check( conditionsCheckContext );
	}
	return conditionsCheckContext.Build();
}

void CAlternativeConditions::Print( ostream& out ) const
{
	if( this->empty() ) {
		return;
	}

	out << "<<";
	bool first = true;
	for( const CAlternativeCondition& condition : *this ) {
		if( first ) {
			first = false;
		} else {
			out << ",";
		}
		condition.Print( out );
	}
	out << ">>";
}

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
	const bool element = main.Values.Find( name.Name, index );

	if( element ) {
		index += name.Index * main.Values.Size(); // correct element index
		if( static_cast<bool>( extendedName.second ) ) {
			CIndexedName subName;
			TSign subIndex;
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
			arg.Reference += name.Index * PatternDefs.size(); // correct reference
			if( main.Values.Find( subName.Name, index ) ) {
				index += subName.Index * main.Values.Size(); // correct element index
				if( arg.Type == PAT_ReferenceElement && arg.Element == index ) {
					return arg;
				}
			} else {
				TSign subIndex;
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
	TSign index;
	if( main.Values.Find( name.Name, index ) ) {
		const TElement element = index + name.Index * main.Values.Size();
		{
			CSignValues values;
			values.Add( index );
			CSignRestriction mainRestriction( element,
				Configuration().WordSigns().MainWordSignIndex(),
				move( values ) );
			const bool added = signRestrictions.Add( move( mainRestriction ) );
			debug_check_logic( added );
		}
		return CPatternBasePtr(
			new CPatternElement( element, move( signRestrictions ) ) );
	} else {
		return CPatternBasePtr( new CPatternReference(
			GetReference( reference ), move( signRestrictions ) ) );
	}
}

TReference CPatternsBuilder::GetReference( const CTokenPtr& reference ) const
{
	const CIndexedName name( reference );
	return PatternReference( name.Name, name.Index );
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
	conditions.Print( out );
}

CPatternBasePtr CAlternativeNode::Check( CPatternsBuilder& context ) const
{
	CPatternBasePtr element = node->Check( context );
	CConditions patternConditions = conditions.Check( context );
	return CPatternBasePtr( new CPatternAlternative( move( element ),
		move( patternConditions ) ) );
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
		CSignRestriction restrictions( arg.Element, arg.Sign,
			move( signValues ), exclude );
		if( restrictions.IsEmpty( context ) ) {
			context.AddComplexError( collectTokens(),
				"words matching the condition, do not exist" );
		}
		if( !signRestrictions.Add( move( restrictions ) ) ) {
			context.AddComplexError( collectTokens(), "duplicate word sign" );
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

				TSign subIndex;
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
		addError( "word or pattern expected" );
		return false;
	}

	name.first = tokens.TokenPtr();
	tokens.Next(); // skip identifier

	if( tokens.MatchType( TT_Dot ) ) {
		if( !tokens.CheckType( TT_Identifier ) ) {
			addError( "word attribute expected" );
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
		addError( "pattern expected" );
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
			addError( "word attribute value expected" );
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

bool CPatternParser::readMatchingCondition( CExtendedNames& names )
{
	do {
		names.emplace_back();
		if( !readExtendedName( names.back() ) ) {
			return false;
		}

		names.emplace_back();
	} while( tokens.MatchType( TT_EqualSign, names.back().second )
		|| tokens.MatchType( TT_DoubleEqualSign, names.back().second ) );

	names.pop_back(); // remove last empty extended name
	return true;
}

// reads TT_Identifier `(` TT_Identifier { TT_Identifier } { `,` TT_Identifier { TT_Identifier } } `)`
bool CPatternParser::readDictionaryCondition( CTokenPtr& dictionary, CExtendedNames& names )
{
	if( !tokens.MatchType( TT_Identifier, dictionary ) ) {
		addError( "dictionary name expected" );
		return false;
	}

	if( !tokens.MatchType( TT_OpeningParenthesis ) ) {
		addError( "opening parenthesis `(` expected" );
		return false;
	}

	while( tokens.CheckType( TT_Identifier ) ) {
		names.emplace_back( tokens.TokenPtr(), nullptr );
		tokens.Next();
		if( tokens.CheckType( TT_Comma ) ) {
			names.emplace_back( nullptr, tokens.TokenPtr() );
			tokens.Next();
		}
	}

	if( names.empty() || !static_cast<bool>( names.back().first ) ) {
		addError( "at least one word expected" );
		return false;
	}

	if( !tokens.MatchType( TT_ClosingParenthesis ) ) {
		addError( "closing parenthesis `)` expected" );
		return false;
	}

	return true;
}

bool CPatternParser::readAlternativeCondition( CAlternativeConditions& conditions )
{
	if( tokens.CheckType( TT_OpeningParenthesis, 1 /* offset */ ) ) {
		CTokenPtr dictionary;
		CExtendedNames names;
		if( !readDictionaryCondition( dictionary, names ) ) {
			return false;
		}
		conditions.emplace_back( dictionary, move( names ) );
	} else {
		CExtendedNames names;
		if( !readMatchingCondition( names ) ) {
			return false;
		}
		conditions.emplace_back( move( names ) );
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

	CAlternativeConditions conditions;
	if( !readAlternativeConditions( conditions ) ) {
		return false;
	}

	if( transposition->size() == 1 ) {
		alternative.reset( new CAlternativeNode(
			move( transposition->front() ), move( conditions ) ) );
	} else {
		alternative.reset( new CAlternativeNode(
			move( transposition ), move( conditions ) ) );
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
