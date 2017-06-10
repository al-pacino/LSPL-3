#include <common.h>
#include <Pattern.h>
#include <Parser.h>
#include <PatternMatch.h>
#include <TranspositionSupport.h>

using namespace Lspl::Configuration;
using namespace Lspl::Text;
using Lspl::Parser::CIndexedName;

namespace Lspl {
namespace Pattern {

///////////////////////////////////////////////////////////////////////////////

CPatternArgument::CPatternArgument() :
	Type( PAT_None ),
	Element( 0 ),
	Reference( 0 ),
	Sign( 0 )
{
}

CPatternArgument::CPatternArgument( const TElement element,
		const TPatternArgumentType type, const TSign sign,
		const TReference reference ) :
	Type( type ),
	Element( element ),
	Reference( reference ),
	Sign( sign )
{
}

bool CPatternArgument::Defined() const
{
	return ( Type != PAT_None );
}

bool CPatternArgument::HasSign() const
{
	return ( Type == PAT_ElementSign || Type == PAT_ReferenceElementSign );
}

void CPatternArgument::RemoveSign()
{
	if( Type == PAT_ElementSign ) {
		Type = PAT_Element;
		Sign = 0;
	} else if( Type == PAT_ReferenceElementSign ) {
		Type = PAT_ReferenceElement;
		Sign = 0;
	}
}

bool CPatternArgument::HasReference() const
{
	return ( Type == PAT_ReferenceElement || Type == PAT_ReferenceElementSign );
}

bool CPatternArgument::Inconsistent( const CPatternArgument& arg ) const
{
	if( !Defined() || !arg.Defined() ) {
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
		out << "." << context.Configuration().Attributes()[Sign].Name( 0 );
	}
}

size_t CPatternArgument::Hasher::operator()(
	const CPatternArgument& arg ) const
{
	return ( arg.Reference ^ ( arg.Element << 16 ) ^ ( arg.Sign << 24 )
		^ ( static_cast<size_t>( arg.Type ) << 30 ) );
}

bool CPatternArgument::Comparator::operator()(
	const CPatternArgument& arg1, const CPatternArgument& arg2 ) const
{
	return ( arg1.Type == arg2.Type
		&& arg1.Element == arg2.Element
		&& arg1.Reference == arg2.Reference
		&& arg1.Sign == arg2.Sign );
}

///////////////////////////////////////////////////////////////////////////////

IPatternBase::IPatternBase()
{
}

IPatternBase::~IPatternBase()
{
}

///////////////////////////////////////////////////////////////////////////////

CBaseVariantPart::CBaseVariantPart()
{
}

CBaseVariantPart::~CBaseVariantPart()
{
}

TElement CBaseVariantPart::Word() const
{
	check_logic( false );
	return 0;
}

string CBaseVariantPart::Regexp() const
{
	check_logic( false );
	return "";
}

TReference CBaseVariantPart::Instance() const
{
	check_logic( false );
	return 0;
}

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

TVariantSize CPatternSequence::MinSizePrediction() const
{
	size_t minSizePrediction = 0;
	for( const CPatternBasePtr& childNode : elements ) {
		minSizePrediction += childNode->MinSizePrediction();
	}
	return Cast<TVariantSize>( min<size_t>( minSizePrediction, MaxVariantSize ) );
}

void CPatternSequence::Build( CPatternBuildContext& context,
	CPatternVariants& variants, const TVariantSize maxSize ) const
{
	vector<CPatternVariants> allSubVariants;
	collectAllSubVariants( context, allSubVariants, maxSize );

	if( allSubVariants.empty() ) {
		return;
	}

	debug_check_logic( allSubVariants.size() == elements.size() );
	context.AddVariants( allSubVariants, variants, maxSize );

	if( !transposition ) {
		return;
	}

	const CTranspositionSupport::CSwaps& swaps =
		CTranspositionSupport::Instance().Swaps( allSubVariants.size() );
	for( const CTranspositionSupport::CSwap& swap : swaps ) {
		swap.Apply( allSubVariants );
		context.AddVariants( allSubVariants, variants, maxSize );
	}
}

void CPatternSequence::collectAllSubVariants( CPatternBuildContext& context,
	vector<CPatternVariants>& allSubVariants,
	const TVariantSize maxSize ) const
{
	allSubVariants.clear();
	if( maxSize == 0 ) {
		return;
	}

	const TVariantSize minSize = MinSizePrediction();
	if( minSize > maxSize ) {
		return;
	}

	allSubVariants.reserve( elements.size() );
	for( const CPatternBasePtr& childNode : elements ) {
		const TVariantSize emsp = childNode->MinSizePrediction();
		const TVariantSize mes = maxSize - minSize + emsp;

		CPatternVariants subVariants;
		childNode->Build( context, subVariants, mes );
		if( subVariants.empty() ) {
			allSubVariants.clear();
			return;
		}

		allSubVariants.push_back( subVariants );
	}
}

///////////////////////////////////////////////////////////////////////////////

CCondition::CCondition( const bool _strong,
		const CPatternArguments& _arguments ) :
	strong( _strong ),
	arguments( move( _arguments ) )
{
	debug_check_logic( arguments.size() == 2 );
	debug_check_logic( arguments[0].HasSign() == arguments[1].HasSign() );
	if( CPatternArgument::Comparator{}( arguments[0], arguments[1] ) ) {
		arguments.pop_back();
	}
}

CCondition::CCondition( const string& _dictionary,
		const CPatternArguments& _arguments ) :
	strong( false ),
	dictionary( _dictionary ),
	arguments( move( _arguments ) )
{
	debug_check_logic( !dictionary.empty() );
	debug_check_logic( !arguments.empty() );
	for( const CPatternArgument& argument : arguments ) {
		debug_check_logic( argument.Type == PAT_None
			|| argument.Type == PAT_Element );
	}
}

void CCondition::Print( const CPatterns& context, ostream& out ) const
{
	if( Agreement() ) {
		arguments[0].Print( context, out );
		out << ( Strong() ? "==" : "=" );
		arguments[SelfAgreement() ? 0 : 1].Print( context, out );
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

CConditions::CConditions( vector<CCondition>&& conditions ) :
	data( move( conditions ) )
{
	for( TVariantSize i = 0; i < data.size(); i++ ) {
		const CPatternArguments& arguments = data[i].Arguments();
		for( TVariantSize j = 0; j < arguments.size(); j++ ) {
			CPatternArgument argument = arguments[j];
			if( argument.Defined() ) {
				argument.RemoveSign();
				indices.insert( make_pair( argument, make_pair( i, j ) ) );
			}
		}
	}
}

bool CConditions::buildLinks( CPatternVariant& variant, CLinks& links ) const
{
	debug_check_logic( links.empty() );
	if( variant.empty() ) {
		return false;
	}

	for( TVariantSize wi = 0; wi < variant.size(); wi++ ) {
		if( !variant[wi].Id.Defined() ) {
			continue;
		}

		auto range = indices.equal_range( variant[wi].Id );
		for( auto ci = range.first; ci != range.second; ++ci ) {
			const pair<TVariantSize, TVariantSize> ciai = ci->second;
			if( data[ciai.first].Agreement() ) {
				auto pair = links.insert( { ciai.first, wi, ciai.second } );
				debug_check_logic( pair.second );
			} else {
				auto pair = links.insert( { ciai.first, ciai.second, wi }  );
				debug_check_logic( pair.second );
			}
		}
	}

	return !links.empty();
}

void CConditions::Apply( CPatternVariant& variant ) const
{
	CLinks links;
	if( !buildLinks( variant, links ) ) {
		// no conditions
		return;
	}

	auto i = links.cbegin();
	while( i != links.cend() ) {
		const CCondition& condition = data[( *i )[0]];
		if( condition.Agreement() ) {
			const TAttribute sign = condition.Arguments().front().Sign;
			if( condition.Strong() ) {
				debug_check_logic( ( *i )[2] <= 1 );
				auto j = i;
				while( ++j != links.cend() && ( *j )[0] == ( *i )[0] ) {
					debug_check_logic( ( *j )[2] <= 1 );
					const CLinks::value_type& di = *i;
					const CLinks::value_type& dj = *j;
					variant[dj[1]].Actions.Add( CActionPtr(
						new CAgreementAction( sign, dj[1] - di[1] ) ) );
					i++;
				}
				i = j;
			} else if( condition.SelfAgreement() ) {
				debug_check_logic( ( *i )[2] == 0 );
				vector<TVariantSize> words;
				words.push_back( ( *i )[1] );
				auto j = i;
				while( ++j != links.cend() && ( *j )[0] == ( *i )[0] ) {
					debug_check_logic( ( *j )[2] == 0 );
					const TVariantSize offset = ( *j )[1];
					variant[offset].Actions.Add( CActionPtr(
						new CAgreementAction( sign, offset, words ) ) );
					words.push_back( offset );
				}
				i = j;
			} else {
				debug_check_logic( ( *i )[2] <= 1 );
				array<vector<TVariantSize>, 2> words;
				words[( *i )[2]].push_back( ( *i )[1] );
				auto j = i;
				while( ++j != links.cend() && ( *j )[0] == ( *i )[0] ) {
					debug_check_logic( ( *j )[2] <= 1 );
					const TVariantSize offset = ( *j )[1];
					const TVariantSize another = ( *j )[2] == 0 ? 1 : 0;
					if( !words[another].empty() ) {
						variant[offset].Actions.Add( CActionPtr(
							new CAgreementAction( sign, offset, words[another] ) ) );
					}
					words[( *j )[2]].push_back( offset );
				}
				i = j;
			}
		} else {
#pragma message( "not implemented yet" )
			check_logic( false );
#if 0
			TVariantSize maxOffset = ( *i )[2];
			vector<TVariantSize> words;
			words.push_back( maxOffset );
			auto j = i;
			while( ++j != links.cend() && ( *j )[0] == ( *i )[0] ) {
				if( ( *j )[1] - ( *i )[1] > 1 ) {
					words.push_back( MaxVariantSize );
				}
				const TVariantSize offset = ( *j )[2];
				words.push_back( offset );
				maxOffset = max( offset, maxOffset );
				i++;
			}
			i = j;
			// TODO: check condition
			const TDictionary dictionary = 999;
			variant[maxOffset].Actions.Add( CActionPtr(
				new CDictionaryAction( dictionary, maxOffset, words ) ) );
#endif
		}
	}
}

void CConditions::Print( const CPatterns& context, ostream& out ) const
{
	if( data.empty() ) {
		return;
	}

	out << "<<";
	bool first = true;
	for( const CCondition& condition : data ) {
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

CPatternAlternative::CPatternAlternative( CPatternBasePtr&& _element,
		CConditions&& _conditions ) :
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

TVariantSize CPatternAlternative::MinSizePrediction() const
{
	return element->MinSizePrediction();
}

void CPatternAlternative::Build( CPatternBuildContext& context,
	CPatternVariants& variants, const TVariantSize maxSize ) const
{
	element->Build( context, variants, maxSize );
	for( CPatternVariant& variant : variants ) {
		conditions.Apply( variant );
	}
	variants.SortAndRemoveDuplicates( context.Patterns() );
}

///////////////////////////////////////////////////////////////////////////////

CPatternAlternatives::CPatternAlternatives( CPatternBasePtrs&& _alternatives ) :
	alternatives( move( _alternatives ) )
{
	debug_check_logic( !alternatives.empty() );
}

void CPatternAlternatives::Print( const CPatterns& context, ostream& out ) const
{
	out << "( ";
	bool first = true;
	for( const CPatternBasePtr& alternative : alternatives ) {
		if( first ) {
			first = false;
		} else {
			out << " | ";
		}
		alternative->Print( context, out );
	}
	out << " )";
}

TVariantSize CPatternAlternatives::MinSizePrediction() const
{
	check_logic( !alternatives.empty() );
	TVariantSize minSizePrediction = MaxVariantSize;
	for( const CPatternBasePtr& alternative : alternatives ) {
		minSizePrediction = min( minSizePrediction, alternative->MinSizePrediction() );
	}
	return minSizePrediction;
}

void CPatternAlternatives::Build( CPatternBuildContext& context,
	CPatternVariants& variants, const TVariantSize maxSize ) const
{
	for( const CPatternBasePtr& alternative : alternatives ) {
		CPatternVariants subVariants;
		alternative->Build( context, subVariants, maxSize );
		variants.insert( variants.end(),
			subVariants.cbegin(), subVariants.cend() );
	}
	variants.SortAndRemoveDuplicates( context.Patterns() );
}

///////////////////////////////////////////////////////////////////////////////

CPatternRepeating::CPatternRepeating( CPatternBasePtr&& _element,
		const TVariantSize _minCount, const TVariantSize _maxCount ) :
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
	out << " }<"
		<< static_cast<size_t>( minCount ) << ","
		<< static_cast<size_t>( maxCount ) << ">";
}

TVariantSize CPatternRepeating::MinSizePrediction() const
{
	return minCount;
}

void CPatternRepeating::Build( CPatternBuildContext& context,
	CPatternVariants& variants, const TVariantSize maxSize ) const
{
	variants.clear();
	debug_check_logic( minCount <= maxCount );

	if( minCount == 0 ) {
		variants.emplace_back();
	}
	if( maxSize == 0 ) {
		return;
	}

	const size_t start = minCount > 0 ? minCount : 1;
	const size_t nmsp = element->MinSizePrediction();
	const size_t nsmsp = nmsp * start;
	if( nsmsp > maxSize ) {
		return;
	}

	size_t finish = min<size_t>( maxCount, maxSize / nmsp );
	const TVariantSize elementMaxSize = Cast<TVariantSize>(
		min<size_t>( finish - nsmsp + nmsp, MaxVariantSize ) );

	CPatternVariants subVariants;
	element->Build( context, subVariants, elementMaxSize );

	vector<CPatternVariants> allSubVariants( start - 1, subVariants );
	for( size_t count = start; count <= finish; count++ ) {
		allSubVariants.push_back( subVariants );
		context.AddVariants( allSubVariants, variants, maxSize );
	}

	return;
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

TVariantSize CPatternRegexp::MinSizePrediction() const
{
	return 1;
}

void CPatternRegexp::Build( CPatternBuildContext& /*context*/,
	CPatternVariants& variants, const TVariantSize maxSize ) const
{
	variants.clear();
	if( maxSize > 0 ) {
		CPatternVariant variant;
		variant.emplace_back( &regexp );
		variant.Parts.push_back( this );
		variants.push_back( variant );
	}
}

TVariantPartType CPatternRegexp::Type() const
{
	return VPR_Regexp;
}

string CPatternRegexp::Regexp() const
{
	return regexp;
}

///////////////////////////////////////////////////////////////////////////////

CSignRestriction::CSignRestriction( const TElement _element,
		const TSign _sign, CSignValues&& _values, const bool _exclude ) :
	element( _element ),
	sign( _sign ),
	exclude( _exclude ),
	values( move( _values ) )
{
	debug_check_logic( !values.IsEmpty() );
}

void CSignRestriction::Intersection( const CSignRestriction& restriction )
{
	debug_check_logic( sign == restriction.Sign() );
	if( exclude && restriction.exclude ) {
		values = CSignValues::Union( values, restriction.values );
	} else if( exclude && !restriction.exclude ) {
		exclude = false;
		values = CSignValues::Difference( restriction.values, values );
	} else if( !exclude && restriction.exclude ) {
		values = CSignValues::Difference( values, restriction.values );
	} else if( !exclude && !restriction.exclude ) {
		values = CSignValues::Intersection( values, restriction.values );
	}
}

bool CSignRestriction::IsEmpty( const CPatterns& context ) const
{
	const CWordAttribute& attribute = context.Configuration().Attributes()[sign];
	if( exclude ) {
		return ( values.Size() == attribute.ValuesCount() );
	} else {
		return values.IsEmpty();
	}
}

void CSignRestriction::Build( CAttributesRestrictionBuilder& builder ) const
{
	builder.AddAttribute( sign, exclude );
	for( CSignValues::SizeType i = 0; i < values.Size(); i++ ) {
		builder.AddAttributeValue( values.Value( i ) );
	}
}

void CSignRestriction::Print( const CPatterns& context, ostream& out ) const
{
	const CWordAttribute& attribute = context.Configuration().Attributes()[sign];
	out << attribute.Name( 0 );
	out << ( exclude ? "!=" : "=" );
	for( TAttributeValue i = 0; i < values.Size(); i++ ) {
		if( i > 0 ) {
			out << "|";
		}
		out << attribute.Value( values.Value( i ) );
	}
}

///////////////////////////////////////////////////////////////////////////////

bool CSignRestrictions::Add( CSignRestriction&& restriction )
{
	struct {
		bool operator()( const CSignRestriction& restriction1,
			const CSignRestriction& restriction2 ) const
		{
			return ( restriction1.Element() < restriction2.Element()
				|| ( restriction1.Element() == restriction2.Element()
					&& restriction1.Sign() < restriction2.Sign() ) );
		}
	} comparator;

	auto i = lower_bound( data.begin(), data.end(), restriction, comparator );
	if( i != data.cend() && !comparator( restriction, *i ) ) { // *i == restriction
		return false;
	}
	data.insert( i, move( restriction ) );
	return true;
}

void CSignRestrictions::Intersection( const CSignRestrictions& restrictions,
	const TElement element )
{
	if( restrictions.data.empty() ) {
		return;
	}

	struct {
		bool operator()( const CSignRestriction& restriction,
			const TElement element ) const
		{
			return restriction.Element() < element;
		}
		bool operator()( const TElement element,
			const CSignRestriction& restriction ) const
		{
			return element < restriction.Element();
		}
	} filter;
	auto range = equal_range( restrictions.data.begin(),
		restrictions.data.end(), element, filter );

	struct {
		bool operator()( const CSignRestriction& restriction1,
			const CSignRestriction& restriction2 ) const
		{
			return restriction1.Sign() < restriction2.Sign();
		}
	} comparator;

	for( auto i = range.first; i != range.second; ++i ) {
		auto j = lower_bound( data.begin(), data.end(), *i, comparator );
		if( j != data.cend() && !comparator( *j, *i ) ) { // *i == *j
			j->Intersection( *i );
		} else {
			data.insert( j, *i );
		}
	}
}

bool CSignRestrictions::IsEmpty( const CPatterns& context ) const
{
	for( const CSignRestriction& signRestriction : data ) {
		if( signRestriction.IsEmpty( context ) ) {
			return true;
		}
	}
	return false;
}

CAttributesRestriction CSignRestrictions::Build(
	const Configuration::CConfiguration& configuration ) const
{
	CAttributesRestrictionBuilder builder( configuration.Attributes().Size() );

	for( const CSignRestriction& signRestriction : data ) {
		signRestriction.Build( builder );
	}

	return builder.Build();
}

void CSignRestrictions::Print( const CPatterns& context, ostream& out ) const
{
	if( data.empty() ) {
		return;
	}

	out << "<";
	bool first = true;
	for( const CSignRestriction& signRestriction : data ) {
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
	element( _element ),
	signs()
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

TVariantSize CPatternElement::MinSizePrediction() const
{
	return 1;
}

void CPatternElement::Build( CPatternBuildContext& /*context*/,
	CPatternVariants& variants, const TVariantSize maxSize ) const
{
	variants.clear();
	if( maxSize > 0 ) {
		CPatternVariant variant;
		variant.emplace_back( CPatternArgument( element ), signs );
		variant.Parts.push_back( this );
		variants.push_back( variant );
	}
}

TVariantPartType CPatternElement::Type() const
{
	return VPR_Word;
}

TElement CPatternElement::Word() const
{
	return element;
}

///////////////////////////////////////////////////////////////////////////////

CPatternReference::CPatternReference( const TReference _reference ) :
	reference( _reference ),
	signs()
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

TVariantSize CPatternReference::MinSizePrediction() const
{
	return 1;
}

void CPatternReference::Build( CPatternBuildContext& context,
	CPatternVariants& variants, const TVariantSize maxSize ) const
{
	const CPattern& pattern = context.Patterns().Pattern( reference );
	pattern.Build( context, variants, maxSize );

	auto last = variants.begin();
	for( auto variant = last; variant != variants.end(); ++variant ) {
		bool isEmpty = false;
		for( CPatternWord& word : *variant ) {
			if( word.Id.Type == PAT_ReferenceElement ) {
				word.Id.Reference = reference;
				// apply SignRestrictions
				word.SignRestrictions.Intersection( signs, word.Id.Element );
				if( word.SignRestrictions.IsEmpty( context.Patterns() ) ) {
					isEmpty = true;
					break;
				}
			} else {
				word.Id = CPatternArgument();
			}
		}
		if( !isEmpty ) {
			*last = move( *variant );
			last->Parts.front() = this;
			++last;
		}
	}
	variants.erase( last, variants.end() );
}

TVariantPartType CPatternReference::Type() const
{
	return VPR_Instance;
}

TReference CPatternReference::Instance() const
{
	return reference;
}

///////////////////////////////////////////////////////////////////////////////

CPattern::CPattern( const string& _name, CPatternBasePtr&& _root,
		const CPatternArguments& _arguments ) :
	name( _name ),
	reference( numeric_limits<TReference>::max() ),
	root( move( _root ) ),
	arguments( _arguments )
{
	debug_check_logic( !name.empty() );
	debug_check_logic( static_cast<bool>( root ) );
}

void CPattern::SetReference( const TReference _reference )
{
	reference = _reference;
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

TVariantSize CPattern::MinSizePrediction() const
{
	return root->MinSizePrediction();
}

void CPattern::Build( CPatternBuildContext& context,
	CPatternVariants& variants, const TVariantSize maxSize ) const
{
	const TVariantSize correctMaxSize = context.PushMaxSize( reference, maxSize );
	root->Build( context, variants, correctMaxSize );
	const TVariantSize topMaxSize = context.PopMaxSize( reference );
	debug_check_logic( topMaxSize == correctMaxSize );

	if( !variants.empty() && variants.front().empty() ) {
		variants.erase( variants.begin() );
	}

	// correct ids and add first part
	const TElement mainSize =
		context.Patterns().Configuration().Attributes().Main().ValuesCount();
	for( CPatternVariant& variant : variants ) {
		variant.Parts.push_front( this );
		variant.Parts.push_back( nullptr );

		for( CPatternWord& word : variant ) {
			if( word.Id.Type != PAT_Element ) {
				continue;
			}
			for( CPatternArguments::size_type i = 0; i < arguments.size(); i++ ) {
				if( word.Id.Element == arguments[i].Element ) {
					word.Id.Type = PAT_ReferenceElement;
					word.Id.Element = word.Id.Element % mainSize + i * mainSize;
					word.Id.Reference = reference;
					break;
				}
			}
		}
	}
}

TVariantPartType CPattern::Type() const
{
	return VPR_Instance;
}

TReference CPattern::Instance() const
{
	return reference;
}

///////////////////////////////////////////////////////////////////////////////

CPatterns::CPatterns( Configuration::CConfigurationPtr _configuration ) :
	configuration( _configuration )
{
	check_logic( static_cast<bool>( configuration ) );
}

const CPattern& CPatterns::Pattern( const TReference reference ) const
{
	return Patterns[reference % Patterns.size()];
}

void CPatterns::Print( ostream& out ) const
{
	for( const CPattern& pattern : Patterns ) {
		pattern.Print( *this, out );
		out << endl;
	}
}

string CPatterns::Element( const TElement element ) const
{
	const CWordAttribute& main = Configuration().Attributes().Main();
	CIndexedName name;
	name.Index = element / main.ValuesCount();
	name.Name = main.Value( element % main.ValuesCount() );
	return name.Normalize();
}

string CPatterns::Reference( const TReference reference ) const
{
	CIndexedName refName;
	refName.Index = reference / Patterns.size();
	refName.Name = Patterns[reference % Patterns.size()].Name();
	return refName.Normalize();
}

TReference CPatterns::PatternReference( const string& name,
	const TReference nameIndex ) const
{
	auto pi = Names.find( name );
	if( pi == Names.cend() ) {
		return numeric_limits<TSign>::max();
	}
	return ( pi->second + nameIndex * Names.size() );
}

///////////////////////////////////////////////////////////////////////////////

CPatternWord::CPatternWord( const string* const regexp ) :
	Regexp( regexp )
{
	debug_check_logic( Regexp != nullptr );
}

CPatternWord::CPatternWord( const CPatternArgument id,
		const CSignRestrictions& signRestrictions ) :
	Id( id ),
	Regexp( nullptr ),
	SignRestrictions( signRestrictions )
{
	debug_check_logic( Id.Type == PAT_Element );
}

void CPatternWord::Build( CPatternBuildContext& context ) const
{
	const TStateIndex state = context.LastVariant.empty()
		? 0 : context.LastVariant.back().second;

	const TStateIndex nextStateIndex = context.States.size();
	context.States.emplace_back();
	context.States.back().Actions = Actions;

	CTransitionPtr transition;
	if( Regexp != nullptr ) {
		transition.reset( new CWordTransition(
			RegexEx( ToStringEx( *Regexp ) ),
			nextStateIndex ) );
	} else {
		transition.reset( new CAttributesTransition(
			SignRestrictions.Build( context.Patterns().Configuration() ),
			nextStateIndex ) );
	}

	context.States[state].Transitions.emplace_back( transition.release() );

	ostringstream oss;
	Print( context.Patterns(), oss );
	context.LastVariant.push_back( make_pair( oss.str(), nextStateIndex ) );
}

void CPatternWord::Print( const CPatterns& context, ostream& out ) const
{
	if( Regexp != nullptr ) {
		out << '"' << *Regexp << '"';
	} else {
		if( Id.Type != PAT_None ) {
			//debug_check_logic( Id.Type == PAT_ReferenceElement );
			Id.Print( context, out );
		}
		SignRestrictions.Print( context, out );
		Actions.Print( context.Configuration(), out );
	}
}

void CPatternVariant::Build( CPatternBuildContext& context ) const
{
	auto i = this->cbegin();
	auto j = context.LastVariant.begin();

	while( i != this->cend() && j != context.LastVariant.end() ) {
		ostringstream oss;
		i->Print( context.Patterns(), oss );
		if( j->first != oss.str() ) {
			break;
		}
		++i;
		++j;
	}
	debug_check_logic( i != this->cend() );
	context.LastVariant.erase( j, context.LastVariant.end() );

	for( ; i != this->cend(); ++i ) {
		i->Build( context );
	}

	context.States[context.LastVariant.back().second].Actions.Add(
		CActionPtr(
			new CSaveAction(
				CVariantParts( Parts.cbegin(), Parts.cend() ) ) ) );
}

void CPatternVariant::Print( const CPatterns& context, ostream& out ) const
{
	for( const CPatternWord& word : *this ) {
		out << " ";
		word.Print( context, out );
	}
#if 0
	out << endl << "   ";
	for( const CBaseVariantPart* const ve : Parts ) {
		if( ve == nullptr ) {
			out << "} ";
		} else {
			switch( ve->Type() ) {
				case VPR_Word:
					out << context.Element( ve->Word() ) << " ";
					break;
				case VPR_Regexp:
					out << ve->Regexp() << " ";
					break;
				case VPR_Instance:
					out << context.Reference( ve->Instance() ) << "{ ";
					break;
			}
		}
	}
#endif
}

///////////////////////////////////////////////////////////////////////////////

void CPatternVariants::Print( const CPatterns& context, ostream& out ) const
{
	for( const CPatternVariant& variant : *this ) {
		variant.Print( context, out );
		out << endl;
	}
}

void CPatternVariants::SortAndRemoveDuplicates( const CPatterns& context )
{
#pragma message( "very inefficient" )
	typedef pair<string, CPatternVariant> CPair;
	vector<CPair> pairs;
	pairs.reserve( this->size() );
	for( CPatternVariant& variant : *this ) {
		ostringstream oss1;
		variant.Print( context, oss1 );
		pairs.push_back( make_pair( oss1.str(), move( variant ) ) );
	}

	struct {
		bool operator()( const CPair& v1, const CPair& v2 )
		{
			return ( v1.first < v2.first );
		}
	} comparatorL;
	sort( pairs.begin(), pairs.end(), comparatorL );

	struct {
		bool operator()( const CPair& v1, const CPair& v2 )
		{
			return ( v1.first == v2.first );
		}
	} comparatorEq;
	auto last = unique( pairs.begin(), pairs.end(), comparatorEq );
	pairs.erase( last, pairs.end() );

	this->clear();
	for( CPair& pair : pairs ) {
		this->emplace_back( move( pair.second ) );
	}
}

void CPatternVariants::Build( CPatternBuildContext& context ) const
{
	for( const CPatternVariant& variant : *this ) {
		variant.Build( context );
	}
}

///////////////////////////////////////////////////////////////////////////////

CPatternBuildContext::CPatternBuildContext( const CPatterns& _patterns ) :
	patterns( _patterns )
{
	data.resize( patterns.Size() );
	States.emplace_back();
}

TVariantSize CPatternBuildContext::PushMaxSize( const TReference reference,
	const TVariantSize maxSize )
{
	stack<TVariantSize>& maxSizes = data[reference % data.size()].MaxSizes;
	if( maxSizes.empty() || maxSize < maxSizes.top() ) {
		maxSizes.push( maxSize );
	} else {
		debug_check_logic( maxSize == maxSizes.top() );
		maxSizes.push( maxSize - 1 );
	}
	return maxSizes.top();
}

TVariantSize CPatternBuildContext::PopMaxSize( const TReference reference )
{
	stack<TVariantSize>& maxSizes = data[reference % data.size()].MaxSizes;
	debug_check_logic( !maxSizes.empty() );
	const TVariantSize topMaxSize = maxSizes.top();
	maxSizes.pop();
	return topMaxSize;
}

///////////////////////////////////////////////////////////////////////////////

void CPatternBuildContext::AddVariants(
	const vector<CPatternVariants>& allSubVariants,
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

bool CPatternBuildContext::nextIndices(
	const vector<CPatternVariants>& allSubVariants, vector<size_t>& indices )
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

///////////////////////////////////////////////////////////////////////////////

} // end of Pattern namespace
} // end of Lspl namespace
