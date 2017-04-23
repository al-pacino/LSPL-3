#pragma once

#include <common.h>
#include <PatternMatcher.h>

namespace Lspl {
namespace Pattern {

///////////////////////////////////////////////////////////////////////////////

CPatternWordCondition::CPatternWordCondition( const TValue offset, const TSign param ) :
	Size( 1 ),
	Strong( true ),
	Param( param ),
	Offsets( new TValue[Size] )
{
	Offsets[0] = offset;
}

CPatternWordCondition::CPatternWordCondition( const TValue offset,
		const vector<TValue>& words, const TSign param ) :
	Size( Cast<TValue>( words.size() ) ),
	Strong( false ),
	Param( param ),
	Offsets( new TValue[Size] )
{
	debug_check_logic( !words.empty() );
	debug_check_logic( words.size() < Max );
	for( TValue i = 0; i < Size; i++ ) {
		if( words[i] < Max ) {
			debug_check_logic( words[i] <= offset );
			Offsets[i] = offset - words[i];
		} else {
			Offsets[i] = Max;
		}
	}
}

CPatternWordCondition::CPatternWordCondition(
	const CPatternWordCondition& another )
{
	*this = another;
}

CPatternWordCondition& CPatternWordCondition::operator=(
	const CPatternWordCondition& another )
{
	Size = another.Size;
	Strong = another.Strong;
	Param = another.Param;
	Offsets = new TValue[Size];
	memcpy( Offsets, another.Offsets, Size * sizeof( TValue ) );
	return *this;
}

CPatternWordCondition::CPatternWordCondition( CPatternWordCondition&& another )
{
	*this = move( another );
}

CPatternWordCondition& CPatternWordCondition::operator=(
	CPatternWordCondition&& another )
{
	Size = another.Size;
	Strong = another.Strong;
	Param = another.Param;
	Offsets = another.Offsets;
	another.Offsets = nullptr;
	return *this;
}

CPatternWordCondition::~CPatternWordCondition()
{
	delete[] Offsets;
}

void CPatternWordCondition::Print( ostream& out ) const
{
	out << Param
		<< ( Strong ? "==" : "=" )
		<< static_cast<uint32_t>( Offsets[0] );
	for( TValue i = 1; i < Size; i++ ) {
		out << "," << static_cast<uint32_t>( Offsets[i] );
	}
}

///////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////

} // end of Pattern namespace
} // end of Lspl namespace
