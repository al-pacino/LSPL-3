#include <common.h>
#include <PatternsFileProcessor.h>
#include <ErrorProcessor.h>

namespace LsplParser {

///////////////////////////////////////////////////////////////////////////////

CPatternsFileProcessor::CPatternsFileProcessor( CErrorProcessor& _errorProcessor ) :
	errorProcessor( _errorProcessor ),
	tokenizer( errorProcessor )
{
	reset();
}

CPatternsFileProcessor::CPatternsFileProcessor( CErrorProcessor& _errorProcessor,
		const string& filename ) :
	errorProcessor( _errorProcessor ),
	tokenizer( errorProcessor )
{
	Open( filename );
}

CPatternsFileProcessor::~CPatternsFileProcessor()
{
	reset();
}

void CPatternsFileProcessor::Open( const string& filename )
{
	reset();
	file.open( filename, ios::in | ios::binary );
	if( !file.is_open() ) {
		errorProcessor.AddError( CError( "file not found", ES_CriticalError ) );
	} else if( !skipEmptyLines() ) {
		errorProcessor.AddError( CError( "file is empty", ES_CriticalError ) );
	}
}

bool CPatternsFileProcessor::IsOpen() const
{
	return file.is_open();
}

void CPatternsFileProcessor::Close()
{
	reset();
}

// The line is a continuation of a previous one if:
// 1. it starts with space or horizontal tab;
// 2. it contains at least one token.
void CPatternsFileProcessor::ReadPattern( CTokens& patternTokens )
{
	patternTokens.clear();

	check_logic( file.is_open() );
	check_logic( !tokenizer.empty() );

	if( lineStartsWithSpace() ) {
		CError error( CSharedFileLine( line, lineNumber ),
			"a pattern definition is required to be"
			" written from the first character of the line" );
		error.LineSegments.emplace_back( 0, tokenizer.front().Offset + 1 );
		errorProcessor.AddError( move( error ) );
	}
	line.clear();

	// read rest lines of pattern
	while( file.good() ) {
		readLine();
		if( !lineStartsWithSpace() ) {
			break;
		}
		if( !tokenizeLine() ) {
			line.clear();
			break;
		}
	}

	patternTokens = move( tokenizer );
	skipEmptyLines();
}

void CPatternsFileProcessor::reset()
{
	if( file.is_open() ) {
		file.close();
	}
	file.clear();
	tokenizer.Reset();
	lineNumber = 0;
	line.clear();
}

// returns true if new tokens have been added
bool CPatternsFileProcessor::tokenizeLine()
{
	const CTokenizer::size_type sizeBefore = tokenizer.size();
	tokenizer.TokenizeLine( CSharedFileLine( line, lineNumber ) );
	return ( tokenizer.size() > sizeBefore );
}

const size_t TabSize = 4;

static void ReplaceTabsWithSpacesInSignleLine( string& line )
{
	string result;
	result.reserve( line.length() );
	size_t offset = 0;
	for( char c : line ) {
		if( ( static_cast<unsigned char>( c ) & 0xC0 ) == 0xC0 ) {
			continue;
		}
		check_logic( c != '\n' && c != '\r' );
		if( c == '\t' ) {
			const size_t spaceCount = TabSize - ( offset % TabSize );
			result += string( spaceCount, ' ' );
			offset += spaceCount;
		} else {
			result += c;
			offset++;
		}
	}
	line = move( result );
}

void CPatternsFileProcessor::readLine()
{
	check_logic( file.good() );

	++lineNumber;
	getline( file, line, '\n' );

	// support windows EOL style "\r\n"
	if( !line.empty() && line.back() == '\r' ) {
		line.pop_back();
	}

	// not so effective...
	ReplaceTabsWithSpacesInSignleLine( line );
}

// skips empty lines (lines without any tokens)
// returns true if not empty line found
bool CPatternsFileProcessor::skipEmptyLines()
{
	tokenizer.Reset();

	while( !tokenizeLine() && !errorProcessor.HasCriticalErrors() && file.good() ) {
		readLine();
	}

	if( tokenizer.empty() || errorProcessor.HasCriticalErrors() ) {
		reset();
		return false;
	}

	return true;
}

// returns true if the line starts with space or tab
bool CPatternsFileProcessor::lineStartsWithSpace() const
{
	return ( !line.empty() && line.front() == ' ' );
}

///////////////////////////////////////////////////////////////////////////////

} // end of Lspl namespace
