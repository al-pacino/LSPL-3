#pragma once

#include <Tokenizer.h>

namespace LsplParser {

///////////////////////////////////////////////////////////////////////////////

typedef pair<CTokenPtr, CTokenPtr> CExtendedName;
typedef vector<CExtendedName> CExtendedNames;

///////////////////////////////////////////////////////////////////////////////

struct CMatchingCondition {
	bool IsStrong;
	CExtendedNames Elements;

	CMatchingCondition() :
		IsStrong( false )
	{
	}

	void Clear()
	{
		IsStrong = false;
		Elements.clear();
	}

	void Print( ostream& out ) const
	{
		for( const CExtendedName& name : Elements ) {
			out << "=";
			if( IsStrong ) {
				out << "=";
			}
			name.first->Print( out );
			if( name.second ) {
				out << ".";
				name.second->Print( out );
			}
		}
	}
};

///////////////////////////////////////////////////////////////////////////////

struct CDictionaryCondition {
	CTokenPtr DictionaryName;
	vector<vector<CTokenPtr>> Arguments;

	void Clear()
	{
		DictionaryName = nullptr;
		Arguments.clear();
	}

	void Print( ostream& out ) const
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
};

///////////////////////////////////////////////////////////////////////////////

struct CAlternativeConditions {
	vector<CMatchingCondition> MatchingConditions;
	vector<CDictionaryCondition> DictionaryConditions;
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
	virtual void MakeVariants( vector<string>& variants ) const = 0;
};

template<typename ChildrenType = CBasePatternNode>
class CPatternNodesSequence : public CBasePatternNode, public vector<unique_ptr<ChildrenType>> {
public:
	virtual ~CPatternNodesSequence() = 0
	{
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
		if( variants.empty() ) {
			throw logic_error( "CTranspositionNode::MakeVariants variants.empty()" );
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

	virtual void MakeVariants( vector<string>& variants ) const override
	{
		vector<vector<string>> allSubVariants;
		CollectAllSubVariants( allSubVariants );
		AddVariants( allSubVariants, variants );
		if( variants.empty() ) {
			throw logic_error( "CElementsNode::MakeVariants variants.empty()" );
		}
	}
};

class CAlternativeNode : public CBasePatternNode, public CAlternativeConditions {
public:
	CAlternativeNode( unique_ptr<CBasePatternNode> childNode ) :
		node( move( childNode ) )
	{
	}

	virtual ~CAlternativeNode()
	{
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
		if( variants.empty() ) {
			throw logic_error( "CAlternativeNode::MakeVariants variants.empty()" );
		}
	}

private:
	unique_ptr<CBasePatternNode> node;

	string getConditions() const
	{
		if( MatchingConditions.empty() && DictionaryConditions.empty() ) {
			return "";
		}
		ostringstream oss;
		oss << "<<";
		bool isFirst = true;
		for( const CMatchingCondition& cond : MatchingConditions ) {
			if( !isFirst ) {
				oss << ",";
			}
			isFirst = false;
			cond.Print( oss );
		}
		for( const CDictionaryCondition& cond : DictionaryConditions ) {
			if( !isFirst ) {
				oss << ",";
			}
			isFirst = false;
			cond.Print( oss );
		}
		oss << ">>";
		return oss.str();
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
		if( variants.empty() ) {
			throw logic_error( "CAlternativesNode::MakeVariants variants.empty()" );
		}
	}
};

class CRepeatingNode : public CBasePatternNode {
public:
	CRepeatingNode( unique_ptr<CAlternativesNode> alternativesNode,
		CTokenPtr minToken = nullptr, CTokenPtr maxToken = nullptr ) :
		node( move( alternativesNode ) ),
		min( minToken ),
		max( maxToken )
	{
		if( min == nullptr && max != nullptr ) {
			throw logic_error( "CRepeatingNode::CRepeatingNode" );
		}
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

		if( maxCount < minCount ) {
			throw logic_error( "CRepeatingNode::MakeVariants maxCount < minCount" );
		}

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
		if( variants.empty() ) {
			throw logic_error( "CRepeatingNode::MakeVariants variants.empty()" );
		}
	}

private:
	unique_ptr<CBasePatternNode> node;
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

private:
	CTokenPtr regexp;
};

///////////////////////////////////////////////////////////////////////////////

struct CElementCondition {
	CTokenPtr Name;
	CTokenPtr Sign;
	vector<CTokenPtr> Values;

	void Clear()
	{
		Name = nullptr;
		Sign = nullptr;
		Values.clear();
	}
};

typedef vector<CElementCondition> CElementConditions;

///////////////////////////////////////////////////////////////////////////////

class CElementNode : public CBasePatternNode, public CElementConditions {
public:
	CElementNode( CTokenPtr elementToken ) :
		element( elementToken )
	{
	}

	virtual ~CElementNode()
	{
	}

	void Print( ostream& out ) const override
	{
		out << " ";
		element->Print( out );
		out << "<";
		bool isFirst = true;
		for( const CElementCondition& cond : *this ) {
			if( !isFirst ) {
				out << ",";
			}
			isFirst = false;
			if( cond.Name ) {
				cond.Name->Print( out );
			}
			if( cond.Sign ) {
				cond.Sign->Print( out );
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

private:
	CTokenPtr element;
};

///////////////////////////////////////////////////////////////////////////////

struct CPatternDefinition {
	CTokenPtr Name;
	CExtendedNames Arguments;
	unique_ptr<CAlternativesNode> Alternatives;
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

} // end of LsplParser namespace
