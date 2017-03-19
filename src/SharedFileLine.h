#pragma once

namespace Lspl {
namespace Parser {

///////////////////////////////////////////////////////////////////////////////

struct CFileLine {
	const string Line;
	const size_t LineNumber;

	CFileLine( const string& line, size_t lineNumber ) :
		Line( line ),
		LineNumber( lineNumber )
	{
	}
};

class CSharedFileLine : public shared_ptr<CFileLine> {
public:
	CSharedFileLine()
	{
	}
	CSharedFileLine( const string& line, size_t lineNumber ) :
		shared_ptr<CFileLine>( new CFileLine( line, lineNumber ) )
	{
	}

	using shared_ptr<CFileLine>::operator bool;
};

///////////////////////////////////////////////////////////////////////////////

struct CLineSegment {
	size_t Offset; // offset in bytes
	size_t Length; // length in bytes

	explicit CLineSegment( size_t offset = numeric_limits<size_t>::max(), size_t length = 1 ) :
		Offset( offset ),
		Length( length )
	{
	}
};

///////////////////////////////////////////////////////////////////////////////

} // end of Parser namespace
} // end of Lspl namespace
