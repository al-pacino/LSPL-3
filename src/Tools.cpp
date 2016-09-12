#include <common.h>
#include <Tools.h>

namespace LsplTools {

///////////////////////////////////////////////////////////////////////////////

void ReplaceTabsWithSpacesInSignleLine( string& line )
{
	string result;
	result.reserve( line.length() );
	size_t offset = 0;
	for( char c : line ) {
		check_logic( c != '\n' && c != '\r' );
		if( c == '\t' ) {
			const size_t spaceCount = TabSize - ( offset % TabSize );
			result += string( spaceCount, ' ' );
			offset += spaceCount;
		} else {
			result += c;
			if( IsByteFirstInUtf8Symbol( c ) ) {
				offset++;
			}
		}
	}
	line = move( result );
}

string::size_type IsValidUtf8( const string& text )
{
	size_t rest = 0;
	for( string::size_type i = 0; i < text.length(); i++ ) {
		if( IsByteFirstInUtf8Symbol( text[i] ) ) {
			if( rest > 0 ) {
				return i;
			}
			for( unsigned char c = text[i]; ( c & 0x80 ) == 0x80; c <<= 1 ) {
				rest++;
			}
			if( rest > 6 ) { // max number of bytes in UTF-8 symbol
				return i;
			}
			check_logic( rest != 1 );
			if( rest > 0 ) {
				rest--;
			}
		} else {
			if( rest > 0 ) {
				rest--;
			} else {
				return i;
			}
		}
	}
	if( rest > 0 ) {
		return text.length();
	}
	return string::npos;
}

string::size_type IsValidText( const string& text )
{
	for( string::size_type i = 0; i < text.length(); i++ ) {
		if( iscntrl( text[i], locale::classic() ) ) {
			return i;
		}
	}
	return string::npos;
}

///////////////////////////////////////////////////////////////////////////////

} // end of LsplTools namespace
