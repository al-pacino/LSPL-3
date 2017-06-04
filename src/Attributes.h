#pragma once

#include <FixedSizeArray.h>

///////////////////////////////////////////////////////////////////////////////

namespace Lspl {
	template<class TYPE, class SOURCE_TYPE>
	inline const TYPE Cast( const SOURCE_TYPE sourceValue )
	{
		// only numeric types are allowed
		static_assert( TYPE() * 2 == SOURCE_TYPE() * 2, "bad cast" );
		const TYPE value = static_cast<TYPE>( sourceValue );
		debug_check_logic( static_cast<SOURCE_TYPE>( value ) == sourceValue );
		debug_check_logic( ( value >= 0 ) == ( sourceValue >= 0 ) );
		return value;
	}
}

///////////////////////////////////////////////////////////////////////////////

namespace Lspl {
namespace Text {

///////////////////////////////////////////////////////////////////////////////

typedef uint8_t TAttribute;
typedef uint32_t TAttributeValue;

const TAttribute MainAttribute = 0;
const TAttribute MaxAttribute = numeric_limits<TAttribute>::max();
const TAttributeValue NullAttributeValue = 0;

///////////////////////////////////////////////////////////////////////////////

class CAttributes {
public:
	explicit CAttributes( const TAttribute attributesCount );
	const TAttribute Size() const { return attributes.Size(); }
	const TAttributeValue Get( const TAttribute attribute ) const;
	void Set( const TAttribute attribute, const TAttributeValue value );

private:
	CFixedSizeArray<TAttributeValue, TAttribute> attributes;
};

inline const TAttributeValue CAttributes::Get( const TAttribute attribute ) const
{
	return attributes[attribute];
}

inline void CAttributes::Set( const TAttribute attribute, const TAttributeValue value )
{
	attributes[attribute] = value;
}

///////////////////////////////////////////////////////////////////////////////

class CAttributesRestriction {
	friend class CAttributesRestrictionBuilder;

public:
	CAttributesRestriction() = default;
	bool IsEmpty() const { return !static_cast<bool>( data ); }
	void Empty() { data.reset( nullptr ); }
	bool Check( const CAttributes& attributes ) const;

private:
	typedef uint8_t TShort;
	typedef TAttributeValue TWide;
	//static_assert( sizeof( TSmall ) == 1, "bad CAttributesRestriction" );
	//static_assert( sizeof( CHeader ) == 2, "bad CAttributesRestriction" );
	struct CHeader {
		TAttribute Attribute : 8;
		bool Exclude : 1;
		bool Wide : 1;
		uint8_t Length : 6;

		explicit CHeader( const TAttribute attribute = 0,
				const bool exclude = false ) :
			Attribute( attribute ),
			Exclude( exclude ),
			Wide( false ),
			Length( 0 )
		{
		}
	};
	unique_ptr<const CHeader> data;

	static bool checkOne( const CAttributes& attributes, const CHeader*& header );
};

///////////////////////////////////////////////////////////////////////////////

class CAttributesRestrictionBuilder {
public:
	explicit CAttributesRestrictionBuilder( const TAttribute attributesCount );
	void AddAttribute( const TAttribute attribute, const bool exclude );
	void AddAttributeValue( const TAttributeValue value );
	CAttributesRestriction Build() const;

private:
	typedef CAttributesRestriction::TWide TWide;
	typedef CAttributesRestriction::TShort TShort;
	typedef CAttributesRestriction::CHeader CHeader;

	const TAttribute attributesCount;
	vector<CHeader> headers;
	vector<TAttributeValue> values;
};

///////////////////////////////////////////////////////////////////////////////

} // end of Text namespace
} // end of Lspl namespace
