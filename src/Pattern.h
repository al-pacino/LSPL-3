#pragma once

#include <Configuration.h>

namespace Lspl {
namespace Pattern {

///////////////////////////////////////////////////////////////////////////////

class CPatterns;

///////////////////////////////////////////////////////////////////////////////

struct CPatternBuildContext {
};

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

class IPatternBase {
	IPatternBase( const IPatternBase& ) = delete;
	IPatternBase& operator=( const IPatternBase& ) = delete;

public:
	IPatternBase() {}
	virtual ~IPatternBase() = 0 {}

	virtual void Print( const CPatterns& patterns, ostream& out ) const = 0;
	virtual size_t MinSizePrediction() const = 0;
	virtual void Build( CPatternBuildContext& context,
		CPatternVariants& variants, const size_t maxSize ) const = 0;
};

typedef unique_ptr<IPatternBase> CPatternBasePtr;
typedef vector<CPatternBasePtr> CPatternBasePtrs;

///////////////////////////////////////////////////////////////////////////////

class CPatternSequence : public IPatternBase {
public:
	explicit CPatternSequence( CPatternBasePtrs&& elements,
		const bool transposition = false );
	~CPatternSequence() override {}

	void Print( const CPatterns& patterns, ostream& out ) const override;
	size_t MinSizePrediction() const override;
	void Build( CPatternBuildContext& context,
		CPatternVariants& variants, const size_t maxSize ) const override;

private:
	const bool transposition;
	const CPatternBasePtrs elements;
};

///////////////////////////////////////////////////////////////////////////////

class CPatternCondition;
typedef list<CPatternCondition> CPatternConditions;

class CPatternAlternative : public IPatternBase {
public:
	explicit CPatternAlternative( CPatternBasePtr&& element );
	CPatternAlternative( CPatternBasePtr&& element,
		CPatternConditions&& conditions );
	~CPatternAlternative() override {}

	void Print( const CPatterns& patterns, ostream& out ) const override;
	size_t MinSizePrediction() const override;
	void Build( CPatternBuildContext& context,
		CPatternVariants& variants, const size_t maxSize ) const override;

private:
	CPatternBasePtr element;
	CPatternConditions conditions;
};

typedef unique_ptr<CPatternAlternative> CPatternAlternativePtr;
typedef vector<CPatternAlternativePtr> CPatternAlternativePtrs;

///////////////////////////////////////////////////////////////////////////////

class CPatternAlternatives : public IPatternBase {
public:
	explicit CPatternAlternatives( CPatternAlternativePtrs&& alternatives );
	~CPatternAlternatives() override {}

	void Print( const CPatterns& patterns, ostream& out ) const override;
	size_t MinSizePrediction() const override;
	void Build( CPatternBuildContext& context,
		CPatternVariants& variants, const size_t maxSize ) const override;

private:
	const CPatternAlternativePtrs alternatives;
};

typedef unique_ptr<CPatternAlternatives> CPatternAlternativesPtr;

///////////////////////////////////////////////////////////////////////////////

class CPatternRepeating : public IPatternBase {
public:
	explicit CPatternRepeating( CPatternAlternativePtr&& element,
		const size_t minCount = 0, const size_t maxCount = 1 );
	~CPatternRepeating() override {}

	void Print( const CPatterns& patterns, ostream& out ) const override;
	size_t MinSizePrediction() const override;
	void Build( CPatternBuildContext& context,
		CPatternVariants& variants, const size_t maxSize ) const override;

private:
	const CPatternAlternativePtr element;
	const size_t minCount;
	const size_t maxCount;
};

///////////////////////////////////////////////////////////////////////////////

class CPatternRegexp : public IPatternBase {
public:
	explicit CPatternRegexp( const string& regexp );
	~CPatternRegexp() override {}

	void Print( const CPatterns& patterns, ostream& out ) const override;
	size_t MinSizePrediction() const override;
	void Build( CPatternBuildContext& context,
		CPatternVariants& variants, const size_t maxSize ) const override;

private:
	string regexp;
};

///////////////////////////////////////////////////////////////////////////////

typedef Configuration::COrderedList<size_t> CSignValues;

struct CSignRestriction {
	typedef size_t TSign;

	CSignRestriction( const TSign sign, CSignValues&& values,
			const bool exclude = false ) :
		Sign( sign ),
		Exclude( exclude ),
		Values( move( values ) )
	{
	}

	const TSign Sign;
	const bool Exclude;
	const CSignValues Values;
};

typedef vector<CSignRestriction> CSignRestrictions;

///////////////////////////////////////////////////////////////////////////////

class CPatternElement : public IPatternBase {
public:
	typedef size_t TElement;

	explicit CPatternElement( const TElement element );
	CPatternElement( const TElement element, CSignRestrictions&& signs );
	~CPatternElement() override {}

	void Print( const CPatterns& patterns, ostream& out ) const override;
	size_t MinSizePrediction() const override;
	void Build( CPatternBuildContext& context,
		CPatternVariants& variants, const size_t maxSize ) const override;

private:
	const TElement element;
	const CSignRestrictions signs;
};

///////////////////////////////////////////////////////////////////////////////

class CPatternReference : public IPatternBase {
public:
	typedef size_t TReference;

	explicit CPatternReference( const TReference reference );
	CPatternReference( const TReference reference, CSignRestrictions&& signs );
	~CPatternReference() override {}

	void Print( const CPatterns& patterns, ostream& out ) const override;
	size_t MinSizePrediction() const override;
	void Build( CPatternBuildContext& context,
		CPatternVariants& variants, const size_t maxSize ) const override;

private:
	const TReference reference;
	const CSignRestrictions signs;
};

///////////////////////////////////////////////////////////////////////////////

// Sample( A7, N7.c, Sub.Pa, SubSub.c ) = A7 N7 Sub SubSub
// Sub( Pa5 ) = Pa5
// SubSub( Pn7 ) = Pn7
enum TPatternArgumentType {
	PAT_None,
	PAT_Element,				// A7
	PAT_ElementSign,			// N7.c
	PAT_ReferenceElement,		// Sub.Pa
	PAT_ReferenceElementSign	// SubSub.c
};

struct CPatternArgument {
	TPatternArgumentType Type;
	CPatternElement::TElement Element;
	CPatternReference::TReference Reference;
	Configuration::CWordSigns::SizeType Sign;

	CPatternArgument() :
		Type( PAT_None ),
		Element( 0 ),
		Reference( 0 ),
		Sign( 0 )
	{
	}

	explicit CPatternArgument( CPatternElement::TElement element,
			const TPatternArgumentType type = PAT_Element,
			const Configuration::CWordSigns::SizeType sign = 0,
			const CPatternReference::TReference reference = 0 ) :
		Type( type ),
		Element( element ),
		Reference( reference ),
		Sign( sign )
	{
	}

	bool HasSign() const;
	bool HasReference() const;
	bool Inconsistent( const CPatternArgument& arg ) const;
	void Print( const CPatterns& patterns, ostream& out ) const;
};

typedef vector<CPatternArgument> CPatternArguments;

///////////////////////////////////////////////////////////////////////////////

class CPatternCondition {
public:
	CPatternCondition( const bool strong, CPatternArguments&& arguments ) :
		Strong( strong ),
		Arguments( move( arguments ) )
	{
		debug_check_logic( Arguments.size() >= 2 );
	}

	CPatternCondition( const string& dictionary, CPatternArguments&& arguments ) :
		Strong( false ),
		Dictionary( dictionary ),
		Arguments( move( arguments ) )
	{
		debug_check_logic( !Dictionary.empty() );
		debug_check_logic( !Arguments.empty() );
	}

	const bool Strong;
	const string Dictionary;
	const CPatternArguments Arguments;
};

///////////////////////////////////////////////////////////////////////////////

class CPattern {
public:
	CPattern( const string& name, const CPatternArguments& arguments,
		CPatternAlternativesPtr&& alternatives );

	const string& Name() const { return name; }
	const CPatternArguments& Arguments() const { return arguments; }

private:
	const string name;
	const CPatternArguments arguments;
	const CPatternAlternativesPtr alternatives;
};

///////////////////////////////////////////////////////////////////////////////

class CPatterns {
	CPatterns( const CPatterns& ) = delete;
	CPatterns& operator=( const CPatterns& ) = delete;

public:
	CPatterns( CPatterns&& ) = default;

	const Configuration::CConfiguration& Configuration() const
	{
		return *configuration;
	}

protected:
	vector<CPattern> Patterns;
	typedef vector<CPattern>::size_type TPatternIndex;
	unordered_map<string, TPatternIndex> Names;
	// string consts

	explicit CPatterns( Configuration::CConfigurationPtr configuration );

private:
	const Configuration::CConfigurationPtr configuration;
};

///////////////////////////////////////////////////////////////////////////////

} // end of Pattern namespace
} // end of Lspl namespace
