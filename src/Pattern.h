#pragma once

#include <Configuration.h>

namespace Lspl {
namespace Pattern {

///////////////////////////////////////////////////////////////////////////////

class CPatterns;
class CPatternVariant;
class CPatternVariants;
class CPatternBuildContext;

///////////////////////////////////////////////////////////////////////////////

typedef size_t TElement;
typedef size_t TReference;
typedef Configuration::CWordSigns::SizeType TSign;

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
	TElement Element;
	TReference Reference;
	TSign Sign;

	CPatternArgument();
	explicit CPatternArgument( const TElement element,
		const TPatternArgumentType type = PAT_Element,
		const TSign sign = 0, const TReference reference = 0 );

	bool Defined() const;
	bool HasSign() const;
	bool HasReference() const;
	bool Inconsistent( const CPatternArgument& arg ) const;
	void Print( const CPatterns& context, ostream& out ) const;

	struct Hasher {
		size_t operator()( const CPatternArgument& arg ) const;
	};
	struct Comparator {
		bool operator()( const CPatternArgument& arg1,
			const CPatternArgument& arg2 ) const;
	};
};

typedef vector<CPatternArgument> CPatternArguments;

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

	void collectAllSubVariants( CPatternBuildContext& context,
		vector<CPatternVariants>& allSubVariants, const size_t maxSize ) const;
};

///////////////////////////////////////////////////////////////////////////////

class CCondition {
public:
	CCondition( const bool strong, const CPatternArguments& arguments );
	CCondition( const string& dictionary, const CPatternArguments& arguments );
	const CPatternArguments& Arguments() const { return arguments; }
	void Print( const CPatterns& context, ostream& out ) const;

private:
	const bool strong;
	const string dictionary;
	const CPatternArguments arguments;
};

class CConditions {
public:
	explicit CConditions( vector<CCondition>&& conditions );

	void Print( const CPatterns& context, ostream& out ) const;

private:
	vector<CCondition> data;
	typedef vector<CCondition>::size_type TIndex;
	unordered_multimap<CPatternArgument, TIndex,
		CPatternArgument::Hasher,
		CPatternArgument::Comparator> indices;
};

///////////////////////////////////////////////////////////////////////////////

class CPatternAlternative : public IPatternBase {
public:
	CPatternAlternative( CPatternBasePtr&& element, CConditions&& conditions );
	~CPatternAlternative() override {}

	// IPatternBase
	void Print( const CPatterns& context, ostream& out ) const override;
	size_t MinSizePrediction() const override;
	void Build( CPatternBuildContext& context,
		CPatternVariants& variants, const size_t maxSize ) const override;

private:
	CPatternBasePtr element;
	CConditions conditions;
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

	CSignRestriction( const TElement element, const TSign sign,
		CSignValues&& values, const bool exclude = false );

	TSign Sign() const { return sign; }
	TElement Element() const { return element; }
	// returns true if an intersection is not empty
	void Intersection( const CSignRestriction& signRestriction );
	// returns true if there are no words matching restriction
	bool IsEmpty( const CPatterns& context ) const;
	void Print( const CPatterns& context, ostream& out ) const;

private:
	TElement element;
	TSign sign;
	bool exclude;
	CSignValues values;
};

///////////////////////////////////////////////////////////////////////////////

class CSignRestrictions {
public:
	// returns true if the sign restriction was added
	bool Add( CSignRestriction&& signRestriction );
	// returns true if an intersection is not empty
	void Intersection( const CSignRestrictions& signRestrictions,
		const TElement element );
	// returns true if all sign restrictions are empty
	bool IsEmpty( const CPatterns& context ) const;
	void Print( const CPatterns& context, ostream& out ) const;

private:
	vector<CSignRestriction> data;
};

///////////////////////////////////////////////////////////////////////////////

class CPatternElement : public IPatternBase {
public:
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
	string Element( const TElement element ) const;
	string Reference( const TReference reference ) const;
	string SignName( const CSignRestriction::TSign sign ) const;
	string SignValue( const CSignRestriction::TSign sign,
		const CSignValues::ValueType value ) const;
	string String( const CSignValues::ValueType index ) const;
	TReference PatternReference( const string& name,
		const TReference nameIndex = 0 ) const;
	const CPattern& ResolveReference( const TReference reference ) const;

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

struct CPatternWord {
	CPatternArgument Id;
	const string* Regexp;
	CSignRestrictions SignRestrictions;

	explicit CPatternWord( const string* const regexp ) :
		Regexp( regexp )
	{
		debug_check_logic( Regexp != nullptr );
	}

	CPatternWord( const CPatternArgument id,
			const CSignRestrictions& signRestrictions ) :
		Id( id ),
		Regexp( nullptr ),
		SignRestrictions( signRestrictions )
	{
		debug_check_logic( Id.Type == PAT_Element );
	}

	void Print( const CPatterns& context, ostream& out ) const;
};

///////////////////////////////////////////////////////////////////////////////

class CPatternVariant : public vector<CPatternWord> {
public:
	CPatternVariant& operator+=( const CPatternVariant& variant )
	{
		this->insert( this->cend(), variant.cbegin(), variant.cend() );
		return *this;
	}

	void Print( const CPatterns& context, ostream& out ) const;
};

///////////////////////////////////////////////////////////////////////////////

class CPatternVariants : public vector<CPatternVariant> {
public:
	void SortAndRemoveDuplicates( const CPatterns& context );
	void Print( const CPatterns& context, ostream& out ) const;
};

///////////////////////////////////////////////////////////////////////////////

class CPatternBuildContext {
public:
	explicit CPatternBuildContext( const CPatterns& patterns );

	const CPatterns& Patterns() const { return patterns; }

	size_t PushMaxSize( const string& name, const size_t maxSize );
	size_t PopMaxSize( const string& name );

	static void AddVariants( const vector<CPatternVariants>& allSubVariants,
		vector<CPatternVariant>& variants, const size_t maxSize );

private:
	const CPatterns& patterns;
	unordered_map<string, stack<size_t>> namedMaxSizes;

	static bool nextIndices( const vector<CPatternVariants>& allSubVariants,
		vector<size_t>& indices );
};

///////////////////////////////////////////////////////////////////////////////

} // end of Pattern namespace
} // end of Lspl namespace
