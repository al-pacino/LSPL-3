#pragma once

#include <Tokenizer.h>
#include <Configuration.h>

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

	explicit CIndexedName( const CTokenPtr token )
	{
		Parse( token );
	}

	bool Parse( const CTokenPtr token )
	{
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

struct CPatternDefinitionCheckContext {
	const Configuration::CConfiguration& Configuration;
	CErrorProcessor& ErrorProcessor;
	vector<CTokenPtr>& PatternReferences;
	unordered_set<string> Elements;

	CPatternDefinitionCheckContext(
			const Configuration::CConfiguration& configuration,
			CErrorProcessor& errorProcessor,
			vector<CTokenPtr>& patternReferences ) :
		Configuration( configuration ),
		ErrorProcessor( errorProcessor ),
		PatternReferences( patternReferences )
	{
	}

	// nullptr used to seperate line segments
	void AddComplexError( const vector<CTokenPtr>& tokens,
		const char* message ) const;

	bool CheckSubName( const CTokenPtr& subNameToken,
		const bool patternReference, size_t& index ) const;
	string CheckExtendedName( const CExtendedName& extendedName ) const;
};

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
	virtual void Check( CPatternDefinitionCheckContext& context ) const = 0;

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

	virtual void Print( ostream& out ) const override
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

	virtual void Check( CPatternDefinitionCheckContext& context ) const override;
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

	virtual void Print( ostream& out ) const override
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

	virtual void Check( CPatternDefinitionCheckContext& context ) const override;
};

inline const CDictionaryCondition&
CAlternativeCondition::DictionaryCondition() const
{
	check_logic( Type() == ACT_DictionaryCondition );
	return static_cast<const CDictionaryCondition&>( *this );
}

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
	virtual void MakeVariants( vector<string>& variants ) const = 0;
	virtual void Check( CPatternDefinitionCheckContext& context ) const = 0;
};

template<typename ChildrenType = CBasePatternNode>
class CPatternNodesSequence : public CBasePatternNode, public vector<unique_ptr<ChildrenType>> {
public:
	virtual ~CPatternNodesSequence() = 0
	{
	}

	virtual void Check( CPatternDefinitionCheckContext& context ) const override
	{
		for( const unique_ptr<ChildrenType>& node : *this ) {
			node->Check( context );
		}
	}

protected:
	void CollectAllSubVariants( vector<vector<string>>& allSubVariants ) const
	{
		allSubVariants.clear();
		allSubVariants.reserve( size() );
		for( const unique_ptr<CBasePatternNode>& childNode : *this ) {
			vector<string> subVariants;
			childNode->MakeVariants( subVariants );
			allSubVariants.push_back( subVariants );
		}
	}

	void AddVariants( const vector<vector<string>>& allSubVariants,
		vector<string>& variants ) const
	{
		vector<size_t> indices( allSubVariants.size(), 0 );
		do {
			string variant;
			for( size_t i = 0; i < indices.size(); i++ ) {
				variant += allSubVariants[i][indices[i]];
			}
			variants.push_back( variant );
		} while( nextIndices( allSubVariants, indices ) );
	}

private:
	static bool nextIndices( const vector<vector<string>>& allSubVariants,
		vector<size_t>& indices )
	{
		for( size_t pos = indices.size(); pos > 0; pos-- ) {
			const size_t realPos = pos - 1;
			if( indices[realPos] < allSubVariants[realPos].size() - 1 ) {
				indices[realPos]++;
				return true;
			} else {
				indices[realPos] = 0;
			}
		}
		return false;
	}
};

class CTranspositionNode : public CPatternNodesSequence<> {
public:
	virtual ~CTranspositionNode()
	{
	}

	virtual void Print( ostream& out ) const override
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

	virtual void MakeVariants( vector<string>& variants ) const override
	{
		vector<vector<string>> allSubVariants;
		CollectAllSubVariants( allSubVariants );
		AddVariants( allSubVariants, variants );

		check_logic( !variants.empty() );
	}
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

	virtual void MakeVariants( vector<string>& variants ) const override
	{
		vector<vector<string>> allSubVariants;
		CollectAllSubVariants( allSubVariants );
		AddVariants( allSubVariants, variants );

		check_logic( !variants.empty() );
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

	virtual void Print( ostream& out ) const override
	{
		node->Print( out );
		out << getConditions();
	}

	virtual void MakeVariants( vector<string>& variants ) const override
	{
		node->MakeVariants( variants );
		const string conditions = getConditions();
		if( !conditions.empty() ) {
			for( string& variant : variants ) {
				variant += conditions;
			}
		}

		check_logic( !variants.empty() );
	}

	virtual void Check( CPatternDefinitionCheckContext& context ) const override;

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

	virtual void Print( ostream& out ) const override
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

	virtual void MakeVariants( vector<string>& variants ) const override
	{
		variants.clear();
		for( const unique_ptr<CAlternativeNode>& alternative : *this ) {
			vector<string> subVariants;
			alternative->MakeVariants( subVariants );
			variants.insert( variants.end(), subVariants.cbegin(), subVariants.cend() );
		}

		check_logic( !variants.empty() );
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
		min( minToken ),
		max( maxToken )
	{
		check_logic( min != nullptr || max == nullptr );
	}

	virtual ~CRepeatingNode()
	{
	}

	virtual void Print( ostream& out ) const override
	{
		out << " {";
		node->Print( out );
		out << "}";
		if( min ) {
			out << "<";
			min->Print( out );
			if( max ) {
				out << ",";
				max->Print( out );
			}
			out << ">";
		}
	}

	virtual void MakeVariants( vector<string>& variants ) const override
	{
		const size_t minCount = getMinCount();
		const size_t maxCount = getMaxCount();

		check_logic( minCount <= maxCount );

		variants.clear();
		vector<string> subVariants;
		node->MakeVariants( subVariants );

		variants.reserve( maxCount - minCount + 1 );
		for( size_t count = minCount; count <= maxCount; count++ ) {
			if( count == 0 ) {
				variants.push_back( "" );
				continue;
			}
			for( const string& variant : subVariants ) {
				string tmp = "";
				for( size_t i = 0; i < count; i++ ) {
					tmp += variant;
				}
				variants.push_back( tmp );
			}
		}

		check_logic( !variants.empty() );
	}

	virtual void Check( CPatternDefinitionCheckContext& context ) const override;

private:
	unique_ptr<CBasePatternNode> node;
	const bool optionalNode;
	CTokenPtr min;
	CTokenPtr max;

	size_t getMinCount() const
	{
		return ( ( min != nullptr ) ? min->Number : 0 );
	}
	size_t getMaxCount() const
	{
		if( max != nullptr ) {
			return max->Number;
		} else if( min != nullptr ) {
			return getMinCount();
		} else {
			return 1;
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

	virtual void Print( ostream& out ) const override
	{
		out << " ";
		regexp->Print( out );
	}

	virtual void MakeVariants( vector<string>& variants ) const override
	{
		variants.clear();
		ostringstream oss;
		Print( oss );
		variants.push_back( oss.str() );
	}

	virtual void Check( CPatternDefinitionCheckContext& context ) const override;

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

	void Check( CPatternDefinitionCheckContext& context,
		const bool patternReference ) const;
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

	virtual void MakeVariants( vector<string>& variants ) const override
	{
		variants.clear();
		ostringstream oss;
		Print( oss );
		variants.push_back( oss.str() );
	}

	virtual void Check( CPatternDefinitionCheckContext& context ) const override;

private:
	CTokenPtr element;
	CElementConditions conditions;
};

///////////////////////////////////////////////////////////////////////////////

struct CPatternDefinition {
	CTokenPtr Name;
	CExtendedNames Arguments;
	unique_ptr<CAlternativesNode> Alternatives;

	string Check( const Configuration::CConfiguration& configuration,
		CErrorProcessor& errorProcessor,
		vector<CTokenPtr>& references ) const;
};

typedef unique_ptr<CPatternDefinition> CPatternDefinitionPtr;

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
