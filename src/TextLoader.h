#pragma once

#include <Text.h>
#include <Pattern.h>

///////////////////////////////////////////////////////////////////////////////

namespace Lspl {
namespace Text {

///////////////////////////////////////////////////////////////////////////////

bool LoadText( const Pattern::CPatterns& context,
	const char* filename, CWords& words );

///////////////////////////////////////////////////////////////////////////////

} // end of Text namespace
} // end of Lspl namespace
