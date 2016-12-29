#pragma once

#include <Tokenizer.h>

namespace LsplParser {

///////////////////////////////////////////////////////////////////////////////

class CBasePatternNode {
	CBasePatternNode( const CBasePatternNode& ) = delete;
	CBasePatternNode& operator=( const CBasePatternNode& ) = delete;

public:
	CBasePatternNode() {}
	virtual ~CBasePatternNode() = 0 {}
	virtual void Print( ostream& out, size_t offset = 0 ) const = 0;
	virtual string Type() const = 0 {}
};

class CPatternNodesSequence : public CBasePatternNode, public vector<unique_ptr<CBasePatternNode>> {
public:
	virtual ~CPatternNodesSequence() = 0 {}
	virtual void Print( ostream& out, size_t offset ) const
	{
		out << string( offset, ' ' ) << Type() << ":" << endl;
		for( const unique_ptr<CBasePatternNode>& node : *this ) {
			node->Print( out, offset + 3 );
		}
	}
};

class CTranspositionNode : public CPatternNodesSequence {
public:
	virtual ~CTranspositionNode() {}

	virtual string Type() const
	{
		return "CTranspositionNode";
	}
};

class CElementsNode : public CPatternNodesSequence {
public:
	virtual ~CElementsNode() {}

	virtual string Type() const
	{
		return "CElementsNode";
	}
};

class CAlternativesNode : public CPatternNodesSequence {
public:
	virtual ~CAlternativesNode() {}

	virtual string Type() const
	{
		return "CAlternativesNode";
	}
};

class CRepeatingNode : public CBasePatternNode {
public:
	CRepeatingNode( unique_ptr<CBasePatternNode> _node, size_t _min = 0, size_t _max = 1 ) :
		node( move( _node ) ),
		min( _min ),
		max( _max )
	{
		check_logic( min <= max );
		check_logic( max > 0 );
	}

	void Print( ostream& out, size_t offset ) const
	{
		out << string( offset, ' ' ) << Type() << " " << min << " " << max << endl;
		node->Print( out, offset + 4 );
	}

	virtual ~CRepeatingNode() {}

	virtual string Type() const
	{
		return "CRepeatingNode";
	}

private:
	unique_ptr<CBasePatternNode> node;
	size_t min;
	size_t max;
};

class CRegexpNode : public CBasePatternNode {
public:
	CRegexpNode( const string& _regexp ) :
		regexp( _regexp )
	{
	}

	virtual ~CRegexpNode() {}

	virtual string Type() const
	{
		return "CRegexpNode";
	}

	void Print( ostream& out, size_t offset ) const
	{
		out << string( offset, ' ' ) << '"' << regexp << '"' << endl;
	}

private:
	string regexp;
};

class CWordNode : public CBasePatternNode {
public:
	CWordNode( const string& _type ) :
		type( _type )
	{
	}

	virtual ~CWordNode() {}

	void Print( ostream& out, size_t offset ) const
	{
		out << string( offset, ' ' ) << type << endl;
	}

	virtual string Type() const
	{
		return "CWordNode";
	}

private:
	string type;
	/* conditions */
};

///////////////////////////////////////////////////////////////////////////////

class CWordOrPatternName {
public:
	CWordOrPatternName()
	{
		Reset();
	}

	void Reset()
	{
		name.clear();
		subName.clear();
		index = 0;
	}

	const string& Name() const { return name; }
	void SetName( const string& newName ) { name = newName; }
	const string& SubName() const { return subName; }
	void SetSubName( const string& newSubName ) { subName = newSubName; }
	size_t Index() const { return index; }
	void SetIndex( const size_t newIndex ) { index = newIndex; }

	void Print( ostream& out ) const
	{
		out << "NAME: " << name << "[" << index << "]";
		if( !subName.empty() ) {
			out << "." << subName;
		}
		out << endl;
	}

private:
	string name;
	string subName;
	size_t index;
};

///////////////////////////////////////////////////////////////////////////////

class CPatternParser {
	CPatternParser( const CPatternParser& ) = delete;
	CPatternParser& operator=( const CPatternParser& ) = delete;

public:
	explicit CPatternParser( CErrorProcessor& errorProcessor );

	void Parse( const CTokens& tokens );

private:
	CErrorProcessor& errorProcessor;
	CTokensList tokens;

	void addError( const string& text );

	bool readWordOrPatternName( CWordOrPatternName& name );

	bool readPattern();
	bool readPatternName();
	bool readPatternArguments();

	bool readElementCondition();
	bool readElementConditions();

	bool readMatchingCondition();
	bool readDictionaryCondition();
	bool readAlternativeCondition();
	bool readAlternativeConditions();

	bool readElement( unique_ptr<CBasePatternNode>& element );
	bool readElements( unique_ptr<CBasePatternNode>& elements );
	bool readTransposition( unique_ptr<CBasePatternNode>& out );
	bool readAlternatives( unique_ptr<CBasePatternNode>& alternatives );

	// next methods check syntax of read extraction patterns
	bool readTextExtractionPrefix();
	bool readTextExtractionPatterns();
	bool readTextExtractionPattern();
	bool readTextExtractionElements();
	bool readTextExtractionElement( const bool required = false );
};

///////////////////////////////////////////////////////////////////////////////

} // end of LsplParser namespace
