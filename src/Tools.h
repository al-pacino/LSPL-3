#pragma once

namespace LsplTools {

///////////////////////////////////////////////////////////////////////////////

const size_t TabSize = 8;

inline bool IsByteFirstInUtf8Symbol( char c )
{
	return ( ( static_cast<unsigned char>( c ) & 0xC0 ) != 0x80 );
}

void ReplaceTabsWithSpacesInSignleLine( string& line );

///////////////////////////////////////////////////////////////////////////////

} // end of LsplTools namespace
