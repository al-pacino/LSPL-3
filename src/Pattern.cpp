#include <common.h>
#include <Pattern.h>

namespace Lspl {
namespace Pattern {

///////////////////////////////////////////////////////////////////////////////

CPatternSequence::CPatternSequence( CPatternBasePtrs&& _elements,
		const bool _transposition ) :
	elements( move( _elements ) ),
	transposition( _transposition )
{
	debug_check_logic( !elements.empty() );
}


void CPatternSequence::Print( const CPatterns& patterns, ostream& out ) const
{
	bool isFirst = true;
	for( const CPatternBasePtr& childNode : elements ) {
		if( !isFirst ) {
			out << " ~";
		}
		isFirst = false;
		childNode->Print( patterns, out );
	}
}

size_t CPatternSequence::MinSizePrediction() const
{
	size_t minSizePrediction = 0;
	for( const CPatternBasePtr& childNode : elements ) {
		minSizePrediction += childNode->MinSizePrediction();
	}
	return minSizePrediction;
}

void CPatternSequence::Build( CPatternBuildContext& context,
	CPatternVariants& variants, const size_t maxSize ) const
{
#if 0
	vector<CPatternVariants> allSubVariants;
	collectAllSubVariants( context, allSubVariants, maxSize );
	if( !allSubVariants.empty() ) {
		debug_check_logic( allSubVariants.size() == elements.size() );
		addVariants( allSubVariants, variants, maxSize );

		const CTranspositionSupport::CSwaps& swaps =
			CTranspositionSupport::Instance().Swaps( allSubVariants.size() );
		for( const CTranspositionSupport::CSwap& swap : swaps ) {
			swap.Apply( allSubVariants );
			addVariants( allSubVariants, variants, maxSize );
		}
	}
#endif
}

#if 0
void CPatternSequence::collectAllSubVariants( CPatternBuildContext& context,
	vector<CPatternVariants>& allSubVariants, const size_t maxSize ) const
{
	allSubVariants.clear();
	if( maxSize == 0 ) {
		return;
	}

	const size_t minSize = MinSizePrediction();
	if( minSize > maxSize ) {
		return;
	}

	allSubVariants.reserve( elements.size() );
	for( const CPatternBasePtr& childNode : elements ) {
		const size_t emsp = childNode->MinSizePrediction();
		const size_t mes = maxSize - minSize + emsp;

		CPatternVariants subVariants;
		childNode->Build( context, subVariants, mes );
		if( subVariants.empty() ) {
			allSubVariants.clear();
			return;
		}

		allSubVariants.push_back( subVariants );
	}
}

void CPatternSequence::addVariants( const vector<CPatternVariants>& allSubVariants,
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

bool CPatternSequence::nextIndices( const vector<CPatternVariants>& allSubVariants,
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
#endif

///////////////////////////////////////////////////////////////////////////////

CPatternAlternative::CPatternAlternative( CPatternBasePtr&& _element ) :
	element( move( _element ) )
{
	debug_check_logic( static_cast<bool>( element ) );
}

CPatternAlternative::CPatternAlternative( CPatternBasePtr&& _element,
		CPatternConditions&& _conditions ) :
	element( move( _element ) ),
	conditions( _conditions )
{
	debug_check_logic( static_cast<bool>( element ) );
	debug_check_logic( !conditions.empty() );
}

void CPatternAlternative::Print( const CPatterns& patterns, ostream& out ) const
{
}

size_t CPatternAlternative::MinSizePrediction() const
{
	return element->MinSizePrediction();
}

void CPatternAlternative::Build( CPatternBuildContext& context,
	CPatternVariants& variants, const size_t maxSize ) const
{
#if 0
	element->Build( context, variants, maxSize );
	variants.SortAndRemoveDuplicates();
	const string conditions = getConditions();
	if( !conditions.empty() ) {
	for( string& variant : variants ) {
	variant += conditions;
	}
	}
#endif
}

///////////////////////////////////////////////////////////////////////////////

CPatternAlternatives::CPatternAlternatives( CPatternAlternativePtrs&& _alternatives ) :
	alternatives( move( _alternatives ) )
{
	debug_check_logic( !alternatives.empty() );
}

void CPatternAlternatives::Print( const CPatterns& patterns, ostream& out ) const
{
}

size_t CPatternAlternatives::MinSizePrediction() const
{
	check_logic( !alternatives.empty() );
	size_t minSizePrediction = numeric_limits<size_t>::max();
	for( const CPatternAlternativePtr& alternative : alternatives ) {
		minSizePrediction = min( minSizePrediction, alternative->MinSizePrediction() );
	}
	return minSizePrediction;
}

void CPatternAlternatives::Build( CPatternBuildContext& context,
	CPatternVariants& variants, const size_t maxSize ) const
{
#if 0
	for( const CPatternAlternativePtr& alternative : alternatives ) {
		CPatternVariants subVariants;
		alternative->Build( context, subVariants, maxSize );
		variants.insert( variants.end(), subVariants.cbegin(), subVariants.cend() );
	}
#endif
}

///////////////////////////////////////////////////////////////////////////////

CPatternRepeating::CPatternRepeating( CPatternAlternativePtr&& _element,
		const size_t _minCount, const size_t _maxCount ) :
	element( move( _element ) ),
	minCount( _minCount ),
	maxCount( _maxCount )
{
	debug_check_logic( static_cast<bool>( element ) );
	debug_check_logic( minCount <= maxCount );
}

void CPatternRepeating::Print( const CPatterns& patterns, ostream& out ) const
{
	out << "{";
	element->Print( patterns, out );
	out << "}<" << minCount << "," << maxCount << ">";
}

size_t CPatternRepeating::MinSizePrediction() const
{
	return minCount;
}

void CPatternRepeating::Build( CPatternBuildContext& context,
	CPatternVariants& variants, const size_t maxSize ) const
{
#if 0
	size_t minCount = minCount;
	size_t maxCount = maxCount;
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

	CPatternVariants subVariants;
	node->Build( context, subVariants, nodeMaxSize );

	vector<CPatternVariants> allSubVariants( minCount, subVariants );
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
#endif
}

///////////////////////////////////////////////////////////////////////////////

CPatternRegexp::CPatternRegexp( const string& _regexp ) :
	regexp( _regexp )
{
	debug_check_logic( !regexp.empty() );
}

void CPatternRegexp::Print( const CPatterns& patterns, ostream& out ) const
{
	out << '"' << regexp << '"';
}

size_t CPatternRegexp::MinSizePrediction() const
{
	return 1;
}

void CPatternRegexp::Build( CPatternBuildContext& context,
	CPatternVariants& variants, const size_t maxSize ) const
{
#if 0
	variants.clear();
	if( maxSize == 0 ) {
		return;
	}
#endif
}

///////////////////////////////////////////////////////////////////////////////

CPatternElement::CPatternElement( const TElement _element ) :
	element( _element )
{
}

CPatternElement::CPatternElement( const TElement _element,
		CSignRestrictions&& _signs ) :
	element( _element ),
	signs( move( _signs ) )
{
	debug_check_logic( !signs.empty() );
}

void CPatternElement::Print( const CPatterns& patterns, ostream& out ) const
{
}

size_t CPatternElement::MinSizePrediction() const
{
	return 1;
}

void CPatternElement::Build( CPatternBuildContext& context,
	CPatternVariants& variants, const size_t maxSize ) const
{
#if 0
	variants.clear();
	if( maxSize == 0 ) {
		return;
	}
#endif
}

///////////////////////////////////////////////////////////////////////////////

CPatternReference::CPatternReference( const TReference _reference ) :
	reference( _reference )
{
}

CPatternReference::CPatternReference( const TReference _reference,
		CSignRestrictions&& _signs ) :
	reference( _reference ),
	signs( move( _signs ) )
{
	debug_check_logic( !signs.empty() );
}

void CPatternReference::Print( const CPatterns& patterns, ostream& out ) const
{
}

size_t CPatternReference::MinSizePrediction() const
{
	return 1;
}

void CPatternReference::Build( CPatternBuildContext& context,
	CPatternVariants& variants, const size_t maxSize ) const
{
#if 0
	variants.clear();
	if( maxSize == 0 ) {
		return;
	}
#endif
}

///////////////////////////////////////////////////////////////////////////////

void CPatternArgument::Print( const CPatterns& patterns, ostream& out ) const
{
}

///////////////////////////////////////////////////////////////////////////////

CPattern::CPattern( const string& _name, const CPatternArguments& _arguments,
		CPatternAlternativesPtr&& _alternatives ) :
	name( _name ),
	arguments( _arguments ),
	alternatives( move( _alternatives ) )
{
	debug_check_logic( !name.empty() );
	debug_check_logic( static_cast<bool>( alternatives ) );
}

///////////////////////////////////////////////////////////////////////////////

CPatterns::CPatterns( Configuration::CConfigurationPtr _configuration ) :
	configuration( _configuration )
{
	check_logic( static_cast<bool>( configuration ) );
}

///////////////////////////////////////////////////////////////////////////////


} // end of Pattern namespace
} // end of Lspl namespace
