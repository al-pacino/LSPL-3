#pragma once

#include <Tokenizer.h>
#include <Configuration.h>

namespace Lspl {
namespace Parser {

///////////////////////////////////////////////////////////////////////////////

class CTranspositionSupport {
	CTranspositionSupport( const CTranspositionSupport& ) = delete;
	CTranspositionSupport& operator=( const CTranspositionSupport& ) = delete;

public:
	static const size_t MaxTranspositionSize = 9;

	struct CSwap : public pair<size_t, size_t> {
		template<typename ValueType>
		void Apply( vector<ValueType>& vect ) const
		{
			debug_check_logic( first < second );
			debug_check_logic( second < vect.size() );
			ValueType tmp = move( vect[first] );
			vect[first] = move( vect[second] );
			vect[second] = move( tmp );
		}
	};
	typedef vector<CSwap> CSwaps;

	static const CTranspositionSupport& Instance();
	const CSwaps& Swaps( const size_t transpositionSize ) const;

private:
	static unique_ptr<CTranspositionSupport> instance; // singleton
	mutable unordered_map<size_t, CSwaps> allSwaps;

	CTranspositionSupport(); // default constructor
	static void fillSwaps( CSwaps& swaps, const size_t size );

	typedef basic_string<unsigned char> CTransposition;
	typedef list<CTransposition> CTranspositions;
	static CTranspositions generate( const CTransposition& first );
	static bool connect( const CTransposition& first,
		const CTransposition& second, CSwaps& swaps );
};

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

struct CPatternDefinitionCheckContext {
	const Configuration::CConfiguration& Configuration;
	CErrorProcessor& ErrorProcessor;
	vector<CTokenPtr>& PatternReferences;
	unordered_set<string> Elements;
	vector<CTokenPtr> ConditionElements;

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

	bool HasElement( const CTokenPtr& elementToken ) const;
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

	void Check( CPatternDefinitionCheckContext& context ) const override;
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

	void Check( CPatternDefinitionCheckContext& context ) const override;
};

inline const CDictionaryCondition&
CAlternativeCondition::DictionaryCondition() const
{
	check_logic( Type() == ACT_DictionaryCondition );
	return static_cast<const CDictionaryCondition&>( *this );
}

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
	virtual void Check( CPatternDefinitionCheckContext& context ) const = 0;

	virtual size_t MinSizePrediction() const = 0;
	virtual void Collect( vector<CPatternVariant>& variants,
		const size_t maxSize ) const = 0;

protected:
	static void AddVariants( const vector<vector<CPatternVariant>>& allSubVariants,
		vector<CPatternVariant>& variants, const size_t maxSize )
	{
		vector<size_t> indices( allSubVariants.size(), 0 );
		do {
			CPatternVariant variant;
			for( size_t i = 0; i < indices.size(); i++ ) {
				variant += allSubVariants[i][indices[i]];
			}
			if( variant.size() <= maxSize ) {
				variants.push_back( variant );
			}
		} while( nextIndices( allSubVariants, indices ) );
	}

private:
	static bool nextIndices( const vector<vector<CPatternVariant>>& allSubVariants,
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

template<typename ChildrenType = CBasePatternNode>
class CPatternNodesSequence : public CBasePatternNode, public vector<unique_ptr<ChildrenType>> {
public:
	virtual ~CPatternNodesSequence() = 0
	{
	}

	void Check( CPatternDefinitionCheckContext& context ) const override
	{
		for( const unique_ptr<ChildrenType>& node : *this ) {
			node->Check( context );
		}
	}

	size_t MinSizePrediction() const override
	{
		size_t minSizePrediction = 0;
		for( const unique_ptr<ChildrenType>& childNode : *this ) {
			minSizePrediction += childNode->MinSizePrediction();
		}
		return minSizePrediction;
	}

protected:
	void CollectAllSubVariants( vector<vector<CPatternVariant>>& allSubVariants,
		const size_t maxSize ) const
	{
		allSubVariants.clear();
		if( maxSize == 0 ) {
			return;
		}

		const size_t minSize = MinSizePrediction();
		if( minSize > maxSize ) {
			return;
		}

		allSubVariants.reserve( size() );
		for( const unique_ptr<ChildrenType>& childNode : *this ) {
			const size_t emsp = childNode->MinSizePrediction();
			const size_t mes = maxSize - minSize + emsp;
			
			vector<CPatternVariant> subVariants;
			childNode->Collect( subVariants, mes );
			if( subVariants.empty() ) {
				allSubVariants.clear();
				return;
			}

			allSubVariants.push_back( subVariants );
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

	void Check( CPatternDefinitionCheckContext& context ) const override;

	void Collect( vector<CPatternVariant>& variants,
		const size_t maxSize ) const override
	{
		vector<vector<CPatternVariant>> allSubVariants;
		CollectAllSubVariants( allSubVariants, maxSize );
		AddVariants( allSubVariants, variants, maxSize );

		const CTranspositionSupport::CSwaps& swaps =
			CTranspositionSupport::Instance().Swaps( allSubVariants.size() );
		for( const CTranspositionSupport::CSwap& swap : swaps ) {
			swap.Apply( allSubVariants );
			AddVariants( allSubVariants, variants, maxSize );
		}
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

	void Collect( vector<CPatternVariant>& variants,
		const size_t maxSize ) const override
	{
		vector<vector<CPatternVariant>> allSubVariants;
		CollectAllSubVariants( allSubVariants, maxSize );
		AddVariants( allSubVariants, variants, maxSize );
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

	void Check( CPatternDefinitionCheckContext& context ) const override;

	size_t MinSizePrediction() const override
	{
		return node->MinSizePrediction();
	}

	void Collect( vector<CPatternVariant>& variants,
		const size_t maxSize ) const override
	{
		node->Collect( variants, maxSize );
		/*const string conditions = getConditions();
		if( !conditions.empty() ) {
			for( string& variant : variants ) {
				variant += conditions;
			}
		}*/
	}

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

	size_t MinSizePrediction() const override
	{
		check_logic( !this->empty() );
		size_t minSizePrediction = numeric_limits<size_t>::max();
		for( const unique_ptr<CAlternativeNode>& alternative : *this ) {
			minSizePrediction = min( minSizePrediction, alternative->MinSizePrediction() );
		}
		return minSizePrediction;
	}

	void Collect( vector<CPatternVariant>& variants,
		const size_t maxSize ) const override
	{
		for( const unique_ptr<CAlternativeNode>& alternative : *this ) {
			vector<CPatternVariant> subVariants;
			alternative->Collect( subVariants, maxSize );
			variants.insert( variants.end(), subVariants.cbegin(), subVariants.cend() );
		}
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

	void Check( CPatternDefinitionCheckContext& context ) const override;

	size_t MinSizePrediction() const override
	{
		return getMinCount();
	}

	void Collect( vector<CPatternVariant>& variants,
		const size_t maxSize ) const override
	{
		size_t minCount = getMinCount();
		size_t maxCount = getMaxCount();
		check_logic( minCount <= maxCount );

		variants.clear();
		if( minCount == 0 ) {
			variants.emplace_back();
			minCount = 1;
		}

		if( maxSize == 0 ) {
			return;
		}

		const size_t nmsp = node->MinSizePrediction();
		const size_t nsmsp = nmsp * minCount;
		if( nsmsp > maxSize ) {
			return;
		}
		maxCount = min( maxCount, maxSize / nmsp );
		const size_t nodeMaxSize = maxSize - nsmsp + nmsp;

		vector<CPatternVariant> subVariants;
		node->Collect( subVariants, nodeMaxSize );

		vector<vector<CPatternVariant>> allSubVariants( minCount, subVariants );
		AddVariants( allSubVariants, variants, maxSize );

		for( size_t count = minCount + 1; count <= maxCount; count++ ) {
			const size_t variantsSize = variants.size();
			for( size_t vi = 0; vi < variantsSize; vi++ ) {
				const CPatternVariant& variant = variants[vi];
				for( const CPatternVariant& subVariant : subVariants ) {
					if( variant.size() + subVariant.size() <= maxSize ) {
						variants.push_back( variant );
						variants.back() += subVariant;
					}
				}
			}
		}
	}

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

	void Check( CPatternDefinitionCheckContext& context ) const override;

	size_t MinSizePrediction() const override
	{
		return 1;
	}

	void Collect( vector<CPatternVariant>& variants,
		const size_t maxSize ) const override
	{
		variants.clear();
		if( maxSize == 0 ) {
			return;
		}

		ostringstream oss;
		Print( oss );
		variants.emplace_back( oss.str() );
	}

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

	void Check( CPatternDefinitionCheckContext& context ) const override;

	size_t MinSizePrediction() const override
	{
		return 1;
	}

	void Collect( vector<CPatternVariant>& variants,
		const size_t maxSize ) const override
	{
		variants.clear();
		if( maxSize == 0 ) {
			return;
		}

		ostringstream oss;
		Print( oss );
		variants.emplace_back( oss.str() );
	}

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
