#pragma once

namespace LsplTools {

///////////////////////////////////////////////////////////////////////////////

const size_t TabSize = 4;

inline bool IsByteFirstInUtf8Symbol( char c )
{
	return ( ( static_cast<unsigned char>( c ) & 0xC0 ) != 0x80 );
}

inline bool IsByteAsciiSymbol( char c )
{
	return ( ( static_cast<unsigned char>( c ) & 0x80 ) == 0x00 );
}

void ReplaceTabsWithSpacesInSignleLine( string& line );

// Validate UTF-8 text
// If text is valid UTF-8 returns string::npos
// Otherwise, returns index of the first invalid byte
string::size_type IsValidUtf8( const string& text );

// Has the text any ASCII control character?
// If the text has no ASCII control characters returns string::npos
// Otherwise, returns index of the first control character
string::size_type IsValidText( const string& text );

///////////////////////////////////////////////////////////////////////////////

} // end of LsplTools namespace
