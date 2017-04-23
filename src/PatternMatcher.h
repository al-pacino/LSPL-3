#pragma once

namespace Lspl {
namespace Pattern {

///////////////////////////////////////////////////////////////////////////////

typedef size_t TSign;

///////////////////////////////////////////////////////////////////////////////

template<class Type, class SourceType>
inline const Type Cast( const SourceType sourceValue )
{
	// only numeric types are allowed
	static_assert( Type() * 2 == SourceType() * 2, "bad cast" ); 
	const Type value = static_cast<Type>( sourceValue );
	debug_check_logic( static_cast<SourceType>( value ) == sourceValue );
	debug_check_logic( ( value >= 0 ) == ( sourceValue >= 0 ) );
	return value;
}

///////////////////////////////////////////////////////////////////////////////

struct CPatternWordCondition {
	typedef uint8_t TValue;
	static const TValue Max = numeric_limits<TValue>::max();

	CPatternWordCondition( const TValue offset, const TSign param );
	CPatternWordCondition( const TValue offset,
		const vector<TValue>& words, const TSign param );
	CPatternWordCondition( const CPatternWordCondition& another );
	CPatternWordCondition& operator=( const CPatternWordCondition& another );
	CPatternWordCondition( CPatternWordCondition&& another );
	CPatternWordCondition& operator=( CPatternWordCondition&& another );
	~CPatternWordCondition();

	void Print( ostream& out ) const;

	TValue Size;
	bool Strong;
	TSign Param;
	TValue* Offsets;
};

///////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////

} // end of Pattern namespace
} // end of Lspl namespace
