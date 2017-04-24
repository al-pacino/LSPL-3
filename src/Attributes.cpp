#include <common.h>
#include <Attributes.h>

///////////////////////////////////////////////////////////////////////////////

namespace Lspl {
namespace Text {

///////////////////////////////////////////////////////////////////////////////

CAttributes::CAttributes( const TAttribute attributesCount ) :
	attributes( attributesCount )
{
	debug_check_logic( attributesCount > 0 );
	for( TAttribute attr = 0; attr < attributes.Size(); attr++ ) {
		attributes[attr] = NullAttributeValue;
	}
}

///////////////////////////////////////////////////////////////////////////////

bool CAttributesRestriction::Check( const CAttributes& attributes ) const
{
	debug_check_logic( !IsEmpty() );
	const CHeader* header = data.get();
	do {
		if( !checkOne( attributes, header ) ) {
			return false;
		}
	} while( header->Length != 0 );
	return true;
}

bool CAttributesRestriction::checkOne( const CAttributes& attributes,
	const CHeader*& header )
{
	const TAttributeValue value = attributes.Get( header->Attribute );
	if( header->Wide ) {
		const TWide* begin = reinterpret_cast<const TWide*>( header + 1 );
		const TWide* const end = begin + header->Length;
		for( ; begin != end; begin++ ) {
			if( *begin == value ) {
				break;
			}
		}
		const bool found = ( begin != end );
		const bool exclude = header->Exclude;
		header = reinterpret_cast<const CHeader*>( end );
		return ( found != exclude );
	} else {
		const TShort* begin = reinterpret_cast<const TShort*>( header + 1 );
		const TShort* const end = begin + header->Length;
		for( ; begin != end; begin++ ) {
			if( *begin == value ) {
				break;
			}
		}
		const bool found = ( begin != end );
		const bool exclude = header->Exclude;
		header = reinterpret_cast<const CHeader*>( end );
		return ( found != exclude );
	}
}

///////////////////////////////////////////////////////////////////////////////

CAttributesRestrictionBuilder::CAttributesRestrictionBuilder(
		const TAttribute _attributesCount )
	: attributesCount( _attributesCount )
{
	debug_check_logic( attributesCount > 0 );
}

void CAttributesRestrictionBuilder::AddAttribute( const TAttribute attribute,
	const bool exclude )
{
	debug_check_logic( attribute < attributesCount );
	if( !headers.empty() ) {
		debug_check_logic( headers.back().Attribute < attribute );
		debug_check_logic( headers.back().Length > 0 );
	}
	headers.emplace_back( attribute, exclude );
}

void CAttributesRestrictionBuilder::AddAttributeValue(
	const TAttributeValue value )
{
	debug_check_logic( !headers.empty() );
	if( headers.back().Length > 0 ) {
		debug_check_logic( !values.empty() && values.back() < value );
	}
	values.push_back( value );
	headers.back().Length++;
	if( value > numeric_limits<TShort>::max() ) {
		headers.back().Wide = true;
	}
}

CAttributesRestriction CAttributesRestrictionBuilder::Build() const
{
	debug_check_logic( !headers.empty() );
	size_t memsize = 0;
	for( const CHeader& header : headers ) {
		debug_check_logic( header.Length > 0 );
		memsize += sizeof( CHeader ) + header.Length
			* ( header.Wide ? sizeof( TWide ) : sizeof( TShort ) );
	}
	debug_check_logic( memsize > 0 );
	memsize += sizeof( CHeader ); // last empty header
	unique_ptr<CHeader> data( reinterpret_cast<CHeader*>( operator new( memsize ) ) );

	auto vi = values.cbegin();
	CHeader* dataPtr = data.get();
	for( const CHeader& header : headers ) {
		new( dataPtr )CHeader( header );
		dataPtr++;
		if( header.Wide ) {
			TWide* valuePtr = reinterpret_cast<TWide*>( dataPtr );
			for( uint8_t i = 0; i < header.Length; i++ ) {
				debug_check_logic( vi != values.cend() );
				*valuePtr = *vi;
				++valuePtr;
				++vi;
			}
			dataPtr = reinterpret_cast<CHeader*>( valuePtr );
		} else {
			TShort* valuePtr = reinterpret_cast<TShort*>( dataPtr );
			for( uint8_t i = 0; i < header.Length; i++ ) {
				debug_check_logic( vi != values.cend() );
				*valuePtr = *vi;
				++valuePtr;
				++vi;
			}
			dataPtr = reinterpret_cast<CHeader*>( valuePtr );
		}
	}
	debug_check_logic( vi == values.cend() );
	new( dataPtr )CHeader;

	CAttributesRestriction attributesRestriction;
	attributesRestriction.data.reset( data.release() );
	return attributesRestriction;
}

///////////////////////////////////////////////////////////////////////////////

} // end of Text namespace
} // end of Lspl namespace
