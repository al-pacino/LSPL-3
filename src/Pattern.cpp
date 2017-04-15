#include <common.h>
#include <Pattern.h>

#include <Parser.h>

using namespace Lspl::Configuration;
using Lspl::Parser::CIndexedName;

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


void CPatternSequence::Print( const CPatterns& context, ostream& out ) const
{
	bool first = true;
	for( const CPatternBasePtr& childNode : elements ) {
		if( first ) {
			first = false;
		} else {
			out << ( transposition ? " ~ " : " " );
		}
		childNode->Print( context, out );
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

void CPatternConditions::Print( const CPatterns& context, ostream& out ) const
{
	if( this->empty() ) {
		return;
	}

	out << "<<";
	bool first = true;
	for( const CPatternCondition& condition : *this ) {
		if( first ) {
			first = false;
		} else {
			out << ",";
		}
		condition.Print( context, out );
	}
	out << ">>";
}

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
}

void CPatternAlternative::Print( const CPatterns& context, ostream& out ) const
{
	element->Print( context, out );
	conditions.Print( context, out );
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

CPatternAlternatives::CPatternAlternatives( CPatternBasePtrs&& _alternatives ) :
	alternatives( move( _alternatives ) )
{
	debug_check_logic( !alternatives.empty() );
}

void CPatternAlternatives::Print( const CPatterns& context, ostream& out ) const
{
	bool first = true;
	for( const CPatternBasePtr& alternative : alternatives ) {
		if( first ) {
			first = false;
		} else {
			out << " | ";
		}
		alternative->Print( context, out );
	}
}

size_t CPatternAlternatives::MinSizePrediction() const
{
	check_logic( !alternatives.empty() );
	size_t minSizePrediction = numeric_limits<size_t>::max();
	for( const CPatternBasePtr& alternative : alternatives ) {
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

CPatternRepeating::CPatternRepeating( CPatternBasePtr&& _element,
		const size_t _minCount, const size_t _maxCount ) :
	element( move( _element ) ),
	minCount( _minCount ),
	maxCount( _maxCount )
{
	debug_check_logic( static_cast<bool>( element ) );
	debug_check_logic( minCount <= maxCount );
	debug_check_logic( maxCount > 0 );
}

void CPatternRepeating::Print( const CPatterns& context, ostream& out ) const
{
	out << "{ ";
	element->Print( context, out );
	out << " }<" << minCount << "," << maxCount << ">";
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

void CPatternRegexp::Print( const CPatterns& /*context*/, ostream& out ) const
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

CSignRestriction::CSignRestriction( const TSign _sign, CSignValues&& _values,
		const bool _exclude ) :
	sign( _sign ),
	exclude( _exclude ),
	values( move( _values ) )
{
	debug_check_logic( !values.IsEmpty() );
}

void CSignRestriction::Print( const CPatterns& context, ostream& out ) const
{
	out << context.SignName( sign );
	out << ( exclude ? "!=" : "=" );
	for( CSignValues::SizeType i = 0; i < values.Size(); i++ ) {
		if( i > 0 ) {
			out << "|";
		}
		out << context.SignValue( sign, values.Value( i ) );
	}
}

void CSignRestrictions::Print( const CPatterns& context, ostream& out ) const
{
	if( this->empty() ) {
		return;
	}

	out << "<";
	bool first = true;
	for( const CSignRestriction& signRestriction : *this ) {
		if( first ) {
			first = false;
		} else {
			out << ",";
		}
		signRestriction.Print( context, out );
	}
	out << ">";
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
}

void CPatternElement::Print( const CPatterns& context, ostream& out ) const
{
	out << context.Element( element );
	signs.Print( context, out );
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
}

void CPatternReference::Print( const CPatterns& context, ostream& out ) const
{
	out << context.Reference( reference );
	signs.Print( context, out );
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

bool CPatternArgument::HasSign() const
{
	return ( Type == PAT_ElementSign || Type == PAT_ReferenceElementSign );
}

bool CPatternArgument::HasReference() const
{
	return ( Type == PAT_ReferenceElement || Type == PAT_ReferenceElementSign );
}

bool CPatternArgument::Inconsistent( const CPatternArgument& arg ) const
{
	if( Type == PAT_None || arg.Type == PAT_None ) {
		return false;
	}
	if( HasSign() != arg.HasSign() ) {
		return true;
	}
	return ( Sign != arg.Sign );
}

void CPatternArgument::Print( const CPatterns& context, ostream& out ) const
{
	debug_check_logic( Type != PAT_None );
	if( HasReference() ) {
		out << context.Reference( Reference ) << ".";
	}
	out << context.Element( Element );
	if( HasSign() ) {
		out << "." << context.SignName( Sign );
	}
}

///////////////////////////////////////////////////////////////////////////////

CPatternCondition::CPatternCondition( const bool _strong,
		CPatternArguments&& _arguments ) :
	strong( _strong ),
	arguments( move( _arguments ) )
{
	debug_check_logic( arguments.size() >= 2 );
}

CPatternCondition::CPatternCondition( const string& _dictionary,
		CPatternArguments&& _arguments ) :
	strong( false ),
	dictionary( _dictionary ),
	arguments( move( _arguments ) )
{
	debug_check_logic( !dictionary.empty() );
	debug_check_logic( !arguments.empty() );
}

void CPatternCondition::Print( const CPatterns& context, ostream& out ) const
{
	if( dictionary.empty() ) {
		bool first = true;
		for( const CPatternArgument& arg : arguments ) {
			if( first ) {
				first = false;
			} else {
				out << ( strong ? "==" : "=" );
			}
			arg.Print( context, out );
		}
	} else {
		out << dictionary << "(";
		bool first = true;
		for( const CPatternArgument& arg : arguments ) {
			if( arg.Type == PAT_None ) {
				out << ", ";
				first = true;
			} else {
				if( first ) {
					first = false;
				} else {
					out << " ";
				}
				arg.Print( context, out );
			}
		}
		out << ")";
	}
}

///////////////////////////////////////////////////////////////////////////////

CPattern::CPattern( const string& _name, CPatternBasePtr&& _root,
		const CPatternArguments& _arguments ) :
	name( _name ),
	root( move( _root ) ),
	arguments( _arguments )
{
	debug_check_logic( !name.empty() );
	debug_check_logic( static_cast<bool>( root ) );
}

void CPattern::Print( const CPatterns& context, ostream& out ) const
{
	out << name;
	if( !arguments.empty() ) {
		out << "( ";
		bool first = true;
		for( const CPatternArgument& arg : arguments ) {
			if( first ) {
				first = false;
			} else {
				out << ", ";
			}
			arg.Print( context, out );
		}
		out << " )";
	}
	out << " = ";
	root->Print( context, out );
	out << endl;
}

size_t CPattern::MinSizePrediction() const
{
	return root->MinSizePrediction();
}

void CPattern::Build( CPatternBuildContext& context,
	CPatternVariants& variants, const size_t maxSize ) const
{
	root->Build( context, variants, maxSize );
}

///////////////////////////////////////////////////////////////////////////////

CPatterns::CPatterns( Configuration::CConfigurationPtr _configuration ) :
	configuration( _configuration )
{
	check_logic( static_cast<bool>( configuration ) );
}

void CPatterns::Print( ostream& out ) const
{
	for( const CPattern& pattern : Patterns ) {
		pattern.Print( *this, out );
	}
}

string CPatterns::Element( const CPatternElement::TElement element ) const
{
	const COrderedStrings& values
		= Configuration().WordSigns().MainWordSign().Values;
	CIndexedName name;
	name.Index = element / values.Size();
	name.Name = values.Value( element % values.Size() );
	return name.Normalize();
}

string CPatterns::Reference( const CPatternReference::TReference reference ) const
{
	CIndexedName refName;
	refName.Index = reference / Patterns.size();
	refName.Name = Patterns[reference % Patterns.size()].Name();
	return refName.Normalize();
}

string CPatterns::SignName( const CSignRestriction::TSign sign ) const
{
	const CWordSigns& signs = Configuration().WordSigns();
	debug_check_logic( sign < signs.Size() );
	return signs[sign].Names.Value( 0 );
}

string CPatterns::SignValue( const CSignRestriction::TSign signIndex,
	const CSignValues::ValueType value ) const
{
	const CWordSigns& signs = Configuration().WordSigns();
	debug_check_logic( signIndex < signs.Size() );
	const CWordSign& sign = signs[signIndex];
	if( sign.Type == WST_String ) {
		return String( value );
	} else {
		debug_check_logic( sign.Type == WST_Main || sign.Type == WST_Enum );
		debug_check_logic( value < sign.Values.Size() );
		return sign.Values.Value( value );
	}
}

string CPatterns::String( const CSignValues::ValueType index ) const
{
	debug_check_logic( index < Strings.size() );
	return Strings[index];
}

///////////////////////////////////////////////////////////////////////////////

} // end of Pattern namespace
} // end of Lspl namespace
