#include <common.h>
#include <ErrorProcessor.h>
#include <Tokenizer.h>
#include <Tools.h>

namespace Lspl {
namespace Parser {

///////////////////////////////////////////////////////////////////////////////

CError::CError( const CToken& token, const string& message,
		TErrorSeverity severity ) :
	Severity( severity ),
	Line( token.Line ),
	Message( message )
{
	LineSegments.push_back( token );
}

void CError::Print( ostream& out ) const
{
	check_logic( static_cast<bool>( Line ) != LineSegments.empty() );

	if( Line ) {
		out << Line->LineNumber << ":";
	}

	out << "error: " << Message;

	if( Line ) {
		out << ":" << endl;
		out << Line->Line << endl;
		out << highlightSymbols() << endl;
	} else {
		out << "." << endl;
	}
}

string CError::highlightSymbols() const
{
	const string& sourceLine = Line->Line;

	vector<bool> highlightsMask( sourceLine.length() );
	vector<bool> superHighlightsMask( sourceLine.length() + 1 );
	size_t lashHighlightedSymbolOffset = 0;

	for( const CLineSegment lineSegment : LineSegments ) {
		check_logic( lineSegment.Length > 0 );
		const size_t first = min( lineSegment.Offset, sourceLine.length() );
		const size_t last = min( first + min( lineSegment.Length,
			sourceLine.length() ) - 1, sourceLine.length() );
		if( last > lashHighlightedSymbolOffset ) {
			lashHighlightedSymbolOffset = last;
		}
		for( size_t i = first; i < last; i++ ) {
			highlightsMask[i] = true;
		}
		superHighlightsMask[last] = true;
	}

	string highlights;
	highlights.reserve( lashHighlightedSymbolOffset + 1 );
	for( size_t i = 0; i < lashHighlightedSymbolOffset; i++ ) {
		if( IsByteFirstInUtf8Symbol( Line->Line[i] ) ) {
			if( superHighlightsMask[i] ) {
				highlights += '^';
			} else if( highlightsMask[i] ) {
				highlights += '~';
			} else {
				highlights += ' ';
			}
		}
	}
	highlights += '^';
	return highlights;
}

///////////////////////////////////////////////////////////////////////////////

CErrorProcessor::CErrorProcessor()
{
	Reset();
}

CErrorProcessor::~CErrorProcessor()
{
}

void CErrorProcessor::Reset()
{
	hasErrors = false;
	hasCriticalErrors = false;
	errors.clear();
}

bool CErrorProcessor::HasAnyErrors() const
{
	return ( hasErrors || hasCriticalErrors );
}

bool CErrorProcessor::HasCriticalErrors() const
{
	return hasCriticalErrors;
}

void CErrorProcessor::AddError( CError&& error )
{
	const size_t index = error.Line ? error.Line->LineNumber : 0;
	if( errors.size() <= index ) {
		errors.resize( index + 1 );
	}
	errors[index].push_back( error );

	hasErrors = true;
	if( error.Severity == ES_CriticalError ) {
		hasCriticalErrors = true;
	}
}

void CErrorProcessor::PrintErrors( ostream& out, const string& filename ) const
{
	for( const vector<CError>& lineErrors : errors ) {
		for( const CError& error : lineErrors ) {
			if( !filename.empty() ) {
				out << filename << ":";
			}
			error.Print( out );
			out << endl;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

} // end of Parser namespace
} // end of Lspl namespace
