#pragma once

namespace LsplTools {

///////////////////////////////////////////////////////////////////////////////

const size_t TabSize = 4;

inline bool IsByteFirstInUtf8Symbol( char c )
{
	return ( ( static_cast<unsigned char>( c ) & 0xC0 ) != 0xC0 );
}

void ReplaceTabsWithSpacesInSignleLine( string& line );

///////////////////////////////////////////////////////////////////////////////

} // end of LsplTools namespace
