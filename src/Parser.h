#pragma once

#include <Tokenizer.h>
#include <Pattern.h>

namespace Lspl {
namespace Parser {

///////////////////////////////////////////////////////////////////////////////

struct CIndexedName {
	string Name;
	size_t Index;

	CIndexedName();
	explicit CIndexedName( const CTokenPtr& token );

	// returns true if name ends with index
	bool Parse( const CTokenPtr& token );
	// returns concatenation of Name and Index
	string Normalize() const;
};

///////////////////////////////////////////////////////////////////////////////

typedef pair<CTokenPtr, CTokenPtr> CExtendedName;
typedef vector<CExtendedName> CExtendedNames;

class CPatternsBuilder;
class CPatternDefinition;
class CConditionsCheckContext;
typedef unique_ptr<CPatternDefinition> CPatternDefinitionPtr;

///////////////////////////////////////////////////////////////////////////////

class CAlternativeCondition {
	CAlternativeCondition( const CAlternativeCondition& ) = delete;
	CAlternativeCondition& operator=( const CAlternativeCondition& ) = delete;

public:
	explicit CAlternativeCondition( CExtendedNames&& names );
	CAlternativeCondition( CTokenPtr dictionary, CExtendedNames&& names );
	CAlternativeCondition( CAlternativeCondition&& ) = default;

	void Check( CConditionsCheckContext& context ) const;
	void Print( ostream& out ) const;

private:
	CTokenPtr dictionary; // Not a nullptr only for dictionary condition
	CExtendedNames names;
	// SampleDictionary( A1 N1, N2 ):
	//   <A1, nullptr> <N1, nullptr> <nullptr, ,> <N2, nullptr>
	// A1.c=A2.c==N3 (invalid, only for illustration)
	//   <A1, c> <nullptr, => <A2, c> <nullptr, ==> <N3, nullptr>

	void checkAgreement( CConditionsCheckContext& context ) const;
	bool checkConsistency( CPatternsBuilder& context,
		bool& strong, Pattern::CPatternArguments& words ) const;
	bool isDoubleEqualSign( const CExtendedName& name ) const;
	void checkDictionary( CConditionsCheckContext& context ) const;
};

///////////////////////////////////////////////////////////////////////////////

class CAlternativeConditions : public vector<CAlternativeCondition> {
public:
	Pattern::CConditions Check( CPatternsBuilder& context ) const;
	void Print( ostream& out ) const;
};

///////////////////////////////////////////////////////////////////////////////

class CBasePatternNode {
	CBasePatternNode( const CBasePatternNode& ) = delete;
	CBasePatternNode& operator=( const CBasePatternNode& ) = delete;

public:
	CBasePatternNode() {}
	virtual ~CBasePatternNode() = 0 {}

	// CBasePatternNode
	virtual void Print( ostream& out ) const = 0;
	virtual Pattern::CPatternBasePtr
		Check( CPatternsBuilder& context ) const = 0;
};

///////////////////////////////////////////////////////////////////////////////

template<typename ChildrenType = CBasePatternNode>
class CPatternNodesSequence : public CBasePatternNode, public vector<unique_ptr<ChildrenType>> {
public:
	~CPatternNodesSequence() override = 0 {}

protected:
	void PrintAll( ostream& out, const char* delimiter ) const
	{
		if( delimiter == 0 ) {
			delimiter = "";
		}
		bool isFirst = true;
		for( const unique_ptr<ChildrenType>& childNode : *this ) {
			if( !isFirst ) {
				out << delimiter;
			}
			isFirst = false;
			childNode->Print( out );
		}
	}

	Pattern::CPatternBasePtrs CheckAll( CPatternsBuilder& context ) const
	{
		Pattern::CPatternBasePtrs all;
		for( const unique_ptr<ChildrenType>& node : *this ) {
			all.emplace_back( move( node->Check( context ) ) );
		}
		return move( all );
	}
};

///////////////////////////////////////////////////////////////////////////////

class CTranspositionNode : public CPatternNodesSequence<> {
public:
	~CTranspositionNode() override {}

	// CBasePatternNode
	void Print( ostream& out ) const override;
	Pattern::CPatternBasePtr Check( CPatternsBuilder& context ) const override;
};

///////////////////////////////////////////////////////////////////////////////

class CElementsNode : public CPatternNodesSequence<> {
public:
	~CElementsNode() override {}

	// CBasePatternNode
	void Print( ostream& out ) const override;
	Pattern::CPatternBasePtr Check( CPatternsBuilder& context ) const override;
};

///////////////////////////////////////////////////////////////////////////////

class CAlternativeNode : public CBasePatternNode {
public:
	CAlternativeNode( unique_ptr<CBasePatternNode> childNode,
			CAlternativeConditions&& conditions ) :
		node( move( childNode ) ),
		conditions( move( conditions ) )
	{
	}
	~CAlternativeNode() override {}

	// CBasePatternNode
	void Print( ostream& out ) const override;
	Pattern::CPatternBasePtr Check( CPatternsBuilder& context ) const override;

private:
	unique_ptr<CBasePatternNode> node;
	CAlternativeConditions conditions;
};

///////////////////////////////////////////////////////////////////////////////

class CAlternativesNode : public CPatternNodesSequence<CAlternativeNode> {
public:
	~CAlternativesNode() override {}

	// CBasePatternNode
	void Print( ostream& out ) const override;
	Pattern::CPatternBasePtr Check( CPatternsBuilder& context ) const override;
};

///////////////////////////////////////////////////////////////////////////////

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

	~CRepeatingNode() override {}

	// CBasePatternNode
	void Print( ostream& out ) const override;
	Pattern::CPatternBasePtr Check( CPatternsBuilder& context ) const override;

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

///////////////////////////////////////////////////////////////////////////////

class CRegexpNode : public CBasePatternNode {
public:
	CRegexpNode( CTokenPtr regexpToken ) :
		regexp( regexpToken )
	{
		debug_check_logic( static_cast<bool>( regexp ) );
	}
	~CRegexpNode() override {}

	// CBasePatternNode
	void Print( ostream& out ) const override;
	Pattern::CPatternBasePtr Check( CPatternsBuilder& context ) const override;

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

	void Check( CPatternsBuilder& context, const CTokenPtr& element,
		Pattern::CSignRestrictions& signRestrictions ) const;

private:
	vector<CTokenPtr> collectTokens() const;
};

typedef vector<CElementCondition> CElementConditions;

///////////////////////////////////////////////////////////////////////////////

class CElementNode : public CBasePatternNode {
public:
	CElementNode( CTokenPtr elementToken ) :
		element( elementToken )
	{
		debug_check_logic( static_cast<bool>( element ) );
	}
	~CElementNode() override {}

	CElementConditions& Conditions() { return conditions; }

	// CBasePatternNode
	void Print( ostream& out ) const override;
	Pattern::CPatternBasePtr Check( CPatternsBuilder& context ) const override;

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

	Pattern::CPatternArgument Argument( const size_t index,
		const CPatternsBuilder& context ) const;

	// CBasePatternNode
	void Print( ostream& out ) const override;
	Pattern::CPatternBasePtr Check( CPatternsBuilder& context ) const override;
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
	Pattern::CPatterns Save();

	// nullptr used to seperate line segments
	void AddComplexError( const vector<CTokenPtr>& tokens,
		const char* message ) const;

	bool HasElement( const CTokenPtr& elementToken ) const;
	Pattern::CPatternArgument CheckExtendedName(
		const CExtendedName& extendedName ) const;
	void CheckPatternExists( const CTokenPtr& reference ) const;
	bool IsPatternReference( const CTokenPtr& reference ) const;
	Pattern::CPatternBasePtr BuildElement( const CTokenPtr& reference,
		Pattern::CSignRestrictions&& signRestrictions ) const;
	Pattern::TReference GetReference( const CTokenPtr& reference ) const;

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

	bool readMatchingCondition( CExtendedNames& names );
	bool readDictionaryCondition( CTokenPtr& dictionary, CExtendedNames& names );
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
