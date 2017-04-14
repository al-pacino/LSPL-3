#pragma once

#include <Tokenizer.h>
#include <Pattern.h>

namespace Lspl {
namespace Parser {

///////////////////////////////////////////////////////////////////////////////

struct CIndexedName {
	string Name;
	size_t Index;

	CIndexedName() :
		Index( 0 )
	{
	}

	explicit CIndexedName( const CTokenPtr& token )
	{
		Parse( token );
	}

	// returns true if name ends with index
	bool Parse( const CTokenPtr& token )
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

	string Normalize() const
	{
		return ( Name + to_string( Index ) );
	}
};

///////////////////////////////////////////////////////////////////////////////

typedef pair<CTokenPtr, CTokenPtr> CExtendedName;
typedef vector<CExtendedName> CExtendedNames;

///////////////////////////////////////////////////////////////////////////////

class CPatternsBuilder;
class CPatternDefinition;
typedef unique_ptr<CPatternDefinition> CPatternDefinitionPtr;

///////////////////////////////////////////////////////////////////////////////

enum TAlternativeConditionType {
	ACT_MatchingCondition,
	ACT_DictionaryCondition
};

struct CMatchingCondition;
struct CDictionaryCondition;

class CAlternativeCondition {
public:
	virtual ~CAlternativeCondition()
	{
	}

	TAlternativeConditionType Type() const
	{
		return type;
	}

	const CMatchingCondition& MatchingCondition() const;
	const CDictionaryCondition& DictionaryCondition() const;

	virtual void Print( ostream& out ) const = 0;
	virtual void Check( CPatternsBuilder& context ) const = 0;

protected:
	CAlternativeCondition( const TAlternativeConditionType conditionType ) :
		type( conditionType )
	{
	}

private:
	TAlternativeConditionType type;
};

typedef vector<unique_ptr<CAlternativeCondition>> CAlternativeConditions;

///////////////////////////////////////////////////////////////////////////////

struct CMatchingCondition : public CAlternativeCondition {
	bool Strong;
	CExtendedNames Elements;

	CMatchingCondition() :
		CAlternativeCondition( ACT_MatchingCondition ),
		Strong( false )
	{
	}

	void Clear()
	{
		Strong = false;
		Elements.clear();
	}

	void Print( ostream& out ) const override
	{
		for( const CExtendedName& name : Elements ) {
			out << "=";
			if( Strong ) {
				out << "=";
			}
			name.first->Print( out );
			if( name.second ) {
				out << ".";
				name.second->Print( out );
			}
		}
	}

	void Check( CPatternsBuilder& context ) const override;
};

inline const CMatchingCondition&
CAlternativeCondition::MatchingCondition() const
{
	check_logic( Type() == ACT_MatchingCondition );
	return static_cast<const CMatchingCondition&>( *this );
}

///////////////////////////////////////////////////////////////////////////////

struct CDictionaryCondition : public CAlternativeCondition {
	CTokenPtr DictionaryName;
	vector<vector<CTokenPtr>> Arguments;

	CDictionaryCondition() :
		CAlternativeCondition( ACT_DictionaryCondition )
	{
	}

	void Clear()
	{
		DictionaryName = nullptr;
		Arguments.clear();
	}

	void Print( ostream& out ) const override
	{
		DictionaryName->Print( out );
		out << "(";
		for( const vector<CTokenPtr>& argument : Arguments ) {
			bool isFirst = true;
			for( const CTokenPtr& token : argument ) {
				if( isFirst ) {
					out << ",";
				} else {
					out << " ";
				}
				isFirst = false;
				token->Print( out );
			}
		}
		out << ")";
	}

	void Check( CPatternsBuilder& context ) const override;
};

inline const CDictionaryCondition&
CAlternativeCondition::DictionaryCondition() const
{
	check_logic( Type() == ACT_DictionaryCondition );
	return static_cast<const CDictionaryCondition&>( *this );
}

///////////////////////////////////////////////////////////////////////////////

class CPatternElementCondition {
public:
	CPatternElementCondition( const CAlternativeCondition* const prototype ) :
		Prototype( prototype )
	{
	}

	const CAlternativeCondition* const Prototype;
	vector<string> DependentNames;
};

class CBasePatternNode;
struct CPatternElement {
	string Name;
	const CBasePatternNode* Node; // CElementNode or CRegexNodep
	list<CPatternElementCondition*> Conditions;
};

///////////////////////////////////////////////////////////////////////////////

struct CPatternVariant : public vector<string> {
public:
	CPatternVariant()
	{
	}

	explicit CPatternVariant( const string& element )
	{
		push_back( element );
	}

	CPatternVariant& operator+=( const CPatternVariant& variant )
	{
		this->insert( this->cend(), variant.cbegin(), variant.cend() );
		return *this;
	}
};

struct CPatternVariants : public vector<CPatternVariant> {
	void SortAndRemoveDuplicates()
	{
		struct {
			bool operator()( const CPatternVariant& v1,
				const CPatternVariant& v2 )
			{
				return ( v1.size() < v2.size() );
			}
		} comparator;
		sort( this->begin(), this->end(), comparator );

		auto last = unique( this->begin(), this->end() );
		this->erase( last, this->end() );
	}
};

///////////////////////////////////////////////////////////////////////////////

class CBasePatternNode {
	CBasePatternNode( const CBasePatternNode& ) = delete;
	CBasePatternNode& operator=( const CBasePatternNode& ) = delete;

public:
	CBasePatternNode()
	{
	}

	virtual ~CBasePatternNode() = 0
	{
	}

	virtual void Print( ostream& out ) const = 0;
	virtual void Check( CPatternsBuilder& context ) const = 0;
};

template<typename ChildrenType = CBasePatternNode>
class CPatternNodesSequence : public CBasePatternNode, public vector<unique_ptr<ChildrenType>> {
public:
	virtual ~CPatternNodesSequence() = 0
	{
	}

	void Check( CPatternsBuilder& context ) const override
	{
		for( const unique_ptr<ChildrenType>& node : *this ) {
			node->Check( context );
		}
	}
};

class CTranspositionNode : public CPatternNodesSequence<> {
public:
	virtual ~CTranspositionNode()
	{
	}

	void Print( ostream& out ) const override
	{
		bool isFirst = true;
		for( const unique_ptr<CBasePatternNode>& childNode : *this ) {
			if( !isFirst ) {
				out << " ~";
			}
			isFirst = false;
			childNode->Print( out );
		}
	}

	void Check( CPatternsBuilder& context ) const override;
};

class CElementsNode : public CPatternNodesSequence<> {
public:
	virtual ~CElementsNode()
	{
	}

	virtual void Print( ostream& out ) const override
	{
		for( const unique_ptr<CBasePatternNode>& childNode : *this ) {
			childNode->Print( out );
		}
	}
};

class CAlternativeNode : public CBasePatternNode {
public:
	CAlternativeNode( unique_ptr<CBasePatternNode> childNode ) :
		node( move( childNode ) )
	{
	}

	virtual ~CAlternativeNode()
	{
	}

	CAlternativeConditions& Conditions()
	{
		return conditions;
	}

	void Print( ostream& out ) const override
	{
		node->Print( out );
		out << getConditions();
	}

	void Check( CPatternsBuilder& context ) const override;

private:
	unique_ptr<CBasePatternNode> node;
	CAlternativeConditions conditions;

	string getConditions() const
	{
		auto condition = conditions.cbegin();
		if( condition != conditions.cend() ) {
			ostringstream oss;
			oss << "<<";
			( *condition )->Print( oss );
			for( ++condition; condition != conditions.cend(); ++condition ) {
				oss << ",";
				( *condition )->Print( oss );
			}
			oss << ">>";
			return oss.str();
		}
		return "";
	}
};

class CAlternativesNode : public CPatternNodesSequence<CAlternativeNode> {
public:
	virtual ~CAlternativesNode()
	{
	}

	void Print( ostream& out ) const override
	{
		bool isFirst = true;
		out << " (";
		for( const unique_ptr<CAlternativeNode>& alternative : *this ) {
			if( !isFirst ) {
				out << " |";
			}
			isFirst = false;
			alternative->Print( out );
		}
		out << ")";
	}
};

class CRepeatingNode : public CBasePatternNode {
public:
	explicit CRepeatingNode( unique_ptr<CAlternativesNode> alternativesNode ) :
		node( move( alternativesNode ) ),
		optionalNode( true )
	{
	}

	CRepeatingNode( unique_ptr<CAlternativesNode> alternativesNode,
			CTokenPtr minToken, CTokenPtr maxToken ) :
		node( move( alternativesNode ) ),
		optionalNode( false ),
		minToken( minToken ),
		maxToken( maxToken )
	{
		check_logic( minToken != nullptr || maxToken == nullptr );
	}

	virtual ~CRepeatingNode()
	{
	}

	void Print( ostream& out ) const override
	{
		out << " {";
		node->Print( out );
		out << "}";
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

	void Check( CPatternsBuilder& context ) const override;

private:
	unique_ptr<CBasePatternNode> node;
	const bool optionalNode;
	CTokenPtr minToken;
	CTokenPtr maxToken;

	size_t getMinCount() const
	{
		return ( ( minToken != nullptr ) ? minToken->Number : 0 );
	}
	size_t getMaxCount() const
	{
		if( maxToken != nullptr ) {
			return maxToken->Number;
		} else {
			return ( optionalNode ? 1 : numeric_limits<size_t>::max() );
		}
	}
};

class CRegexpNode : public CBasePatternNode {
public:
	CRegexpNode( CTokenPtr regexpToken ) :
		regexp( regexpToken )
	{
	}

	virtual ~CRegexpNode()
	{
	}

	void Print( ostream& out ) const override
	{
		out << " ";
		regexp->Print( out );
	}

	void Check( CPatternsBuilder& context ) const override;

private:
	CTokenPtr regexp;
};

///////////////////////////////////////////////////////////////////////////////

struct CElementCondition {
	CTokenPtr Name;
	CTokenPtr EqualSign;
	vector<CTokenPtr> Values;

	void Clear()
	{
		Name = nullptr;
		EqualSign = nullptr;
		Values.clear();
	}

	void Check( CPatternsBuilder& context, const bool patternReference ) const;
};

typedef vector<CElementCondition> CElementConditions;

///////////////////////////////////////////////////////////////////////////////

class CElementNode : public CBasePatternNode {
public:
	CElementNode( CTokenPtr elementToken ) :
		element( elementToken )
	{
	}

	virtual ~CElementNode()
	{
	}

	CElementConditions& Conditions()
	{
		return conditions;
	}

	void Print( ostream& out ) const override
	{
		out << " ";
		element->Print( out );
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

	void Check( CPatternsBuilder& context ) const override;

private:
	CTokenPtr element;
	CElementConditions conditions;
};

///////////////////////////////////////////////////////////////////////////////

class CPatternDefinition : public CBasePatternNode {
public:
	CTokenPtr Name;
	CExtendedNames Arguments;
	unique_ptr<CAlternativesNode> Alternatives;

	void Print( ostream& out ) const override
	{
		Alternatives->Print( out );
		out << endl;
	}

	void Check( CPatternsBuilder& context ) const override;

#if 0
	void Build( CPatternDefinitionBuildContext& context,
		CPatternVariants& variants, const size_t maxSize ) const
	{
		size_t correctMaxSize = maxSize;
		if( !maxSizes.empty() ) {
			if( correctMaxSize == maxSizes.top() ) {
				correctMaxSize--;
			} else {
				debug_check_logic( correctMaxSize < maxSizes.top() );
			}
		}
		maxSizes.push( correctMaxSize );
		//Alternatives->Build( context, variants, correctMaxSize );
		debug_check_logic( maxSizes.top() == correctMaxSize );
		maxSizes.pop();
	}

	mutable stack<size_t> maxSizes;
#endif
};

///////////////////////////////////////////////////////////////////////////////

class CPatternsBuilder : public Pattern::CPatterns {
public:
	CPatternsBuilder( Configuration::CConfigurationPtr configuration,
		CErrorProcessor& errorProcessor ) :
		Pattern::CPatterns( configuration ),
		ErrorProcessor( errorProcessor )
	{
	}

	void Read( const char* filename );
	void Check();
	Pattern::CPatterns&& Save();

	// nullptr used to seperate line segments
	void AddComplexError( const vector<CTokenPtr>& tokens,
		const char* message ) const;

	bool HasElement( const CTokenPtr& elementToken ) const;
	bool CheckSubName( const CTokenPtr& subNameToken,
		const bool patternReference, size_t& index ) const;
	bool CheckSubName( const CTokenPtr& subNameToken,
		const bool patternReference, string& name ) const;
	string CheckExtendedName( const CExtendedName& extendedName ) const;

	void CheckPatternExists( const CTokenPtr& reference ) const;
	bool IsPatternReference( const CTokenPtr& reference ) const;

////////

	CErrorProcessor& ErrorProcessor;
	vector<CPatternDefinitionPtr> PatternDefs;

	unordered_set<string> Elements;
	vector<CTokenPtr> ConditionElements;
};

///////////////////////////////////////////////////////////////////////////////

class CPatternParser {
	CPatternParser( const CPatternParser& ) = delete;
	CPatternParser& operator=( const CPatternParser& ) = delete;

public:
	explicit CPatternParser( CErrorProcessor& errorProcessor );

	CPatternDefinitionPtr Parse( const CTokens& tokens );

private:
	CErrorProcessor& errorProcessor;
	CTokensList tokens;

	void addError( const string& text );

	bool readExtendedName( CExtendedName& name );

	bool readPattern( CPatternDefinition& patternDef );
	bool readPatternName( CPatternDefinition& patternDef );
	bool readPatternArguments( CPatternDefinition& patternDef );

	bool readElementCondition( CElementCondition& elementCondition );
	bool readElementConditions( CElementConditions& elementConditions );

	bool readMatchingCondition( CMatchingCondition& condition );
	bool readDictionaryCondition( CDictionaryCondition& condition );
	bool readAlternativeCondition( CAlternativeConditions& conditions );
	bool readAlternativeConditions( CAlternativeConditions& conditions );

	bool readElement( unique_ptr<CBasePatternNode>& element );
	bool readElements( unique_ptr<CBasePatternNode>& elements );
	bool readAlternative( unique_ptr<CAlternativeNode>& alternative );
	bool readAlternatives( unique_ptr<CAlternativesNode>& alternatives );

	// next methods check syntax of read extraction patterns
	bool readTextExtractionPrefix();
	bool readTextExtractionPatterns();
	bool readTextExtractionPattern();
	bool readTextExtractionElements();
	bool readTextExtractionElement( const bool required = false );
};

///////////////////////////////////////////////////////////////////////////////

} // end of Parser namespace
} // end of Lspl namespace
