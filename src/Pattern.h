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
	IPatternBase( IPatternBase&& ) = default;
	virtual ~IPatternBase() = 0 {}

	virtual void Print( const CPatterns& context, ostream& out ) const = 0;
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

	// IPatternBase
	void Print( const CPatterns& context, ostream& out ) const override;
	size_t MinSizePrediction() const override;
	void Build( CPatternBuildContext& context,
		CPatternVariants& variants, const size_t maxSize ) const override;

private:
	const bool transposition;
	const CPatternBasePtrs elements;
};

///////////////////////////////////////////////////////////////////////////////

class CPatternCondition;
class CPatternConditions : public list<CPatternCondition> {
public:
	void Print( const CPatterns& context, ostream& out ) const;
};

///////////////////////////////////////////////////////////////////////////////

class CPatternAlternative : public IPatternBase {
public:
	explicit CPatternAlternative( CPatternBasePtr&& element );
	CPatternAlternative( CPatternBasePtr&& element,
		CPatternConditions&& conditions );
	~CPatternAlternative() override {}

	// IPatternBase
	void Print( const CPatterns& context, ostream& out ) const override;
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
	explicit CPatternAlternatives( CPatternBasePtrs&& alternatives );
	~CPatternAlternatives() override {}

	// IPatternBase
	void Print( const CPatterns& context, ostream& out ) const override;
	size_t MinSizePrediction() const override;
	void Build( CPatternBuildContext& context,
		CPatternVariants& variants, const size_t maxSize ) const override;

private:
	const CPatternBasePtrs alternatives;
};

///////////////////////////////////////////////////////////////////////////////

class CPatternRepeating : public IPatternBase {
public:
	explicit CPatternRepeating( CPatternBasePtr&& element,
		const size_t minCount = 0, const size_t maxCount = 1 );
	~CPatternRepeating() override {}

	// IPatternBase
	void Print( const CPatterns& context, ostream& out ) const override;
	size_t MinSizePrediction() const override;
	void Build( CPatternBuildContext& context,
		CPatternVariants& variants, const size_t maxSize ) const override;

private:
	const CPatternBasePtr element;
	const size_t minCount;
	const size_t maxCount;
};

///////////////////////////////////////////////////////////////////////////////

class CPatternRegexp : public IPatternBase {
public:
	explicit CPatternRegexp( const string& regexp );
	~CPatternRegexp() override {}

	// IPatternBase
	void Print( const CPatterns& context, ostream& out ) const override;
	size_t MinSizePrediction() const override;
	void Build( CPatternBuildContext& context,
		CPatternVariants& variants, const size_t maxSize ) const override;

private:
	string regexp;
};

///////////////////////////////////////////////////////////////////////////////

typedef Configuration::COrderedList<size_t> CSignValues;

class CSignRestriction {
public:
	typedef size_t TSign;

	CSignRestriction( const TSign sign, CSignValues&& values,
		const bool exclude = false );
	void Print( const CPatterns& context, ostream& out ) const;

private:
	const TSign sign;
	const bool exclude;
	const CSignValues values;
};

class CSignRestrictions : public vector<CSignRestriction> {
public:
	void Print( const CPatterns& context, ostream& out ) const;
};

///////////////////////////////////////////////////////////////////////////////

class CPatternElement : public IPatternBase {
public:
	typedef size_t TElement;

	explicit CPatternElement( const TElement element );
	CPatternElement( const TElement element, CSignRestrictions&& signs );
	~CPatternElement() override {}

	// IPatternBase
	void Print( const CPatterns& context, ostream& out ) const override;
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

	// IPatternBase
	void Print( const CPatterns& context, ostream& out ) const override;
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

	explicit CPatternArgument( const CPatternElement::TElement element,
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
	void Print( const CPatterns& context, ostream& out ) const;
};

typedef vector<CPatternArgument> CPatternArguments;

///////////////////////////////////////////////////////////////////////////////

class CPatternCondition {
public:
	CPatternCondition( const bool strong, CPatternArguments&& arguments );
	CPatternCondition( const string& dictionary, CPatternArguments&& arguments );
	void Print( const CPatterns& context, ostream& out ) const;

private:
	const bool strong;
	const string dictionary;
	const CPatternArguments arguments;
};

///////////////////////////////////////////////////////////////////////////////

class CPattern : public IPatternBase {
public:
	CPattern( const string& name, CPatternBasePtr&& root,
		const CPatternArguments& arguments );
	CPattern( CPattern&& ) = default;

	const string& Name() const { return name; }
	const CPatternArguments& Arguments() const { return arguments; }

	// IPatternBase
	void Print( const CPatterns& context, ostream& out ) const override;
	size_t MinSizePrediction() const override;
	void Build( CPatternBuildContext& context,
		CPatternVariants& variants, const size_t maxSize ) const override;

private:
	string name;
	CPatternBasePtr root;
	CPatternArguments arguments;
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

	void Print( ostream& out ) const;
	string Element( const CPatternElement::TElement element ) const;
	string Reference( const CPatternReference::TReference reference ) const;
	string SignName( const CSignRestriction::TSign sign ) const;
	string SignValue( const CSignRestriction::TSign sign,
		const CSignValues::ValueType value ) const;
	string String( const CSignValues::ValueType index ) const;

protected:
	vector<CPattern> Patterns;
	typedef vector<CPattern>::size_type TPatternIndex;
	unordered_map<string, TPatternIndex> Names;

	vector<string> Strings;
	unordered_map<string, CSignValues::ValueType> StringIndices;

	explicit CPatterns( Configuration::CConfigurationPtr configuration );

private:
	const Configuration::CConfigurationPtr configuration;
};

///////////////////////////////////////////////////////////////////////////////

} // end of Pattern namespace
} // end of Lspl namespace
