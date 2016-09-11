#include <common.h>
#include <ErrorProcessor.h>
#include <Tools.h>

namespace LsplParser {

///////////////////////////////////////////////////////////////////////////////

void CError::Print( ostream& out ) const
{
	check_logic( static_cast<bool>( Line ) != LineSegments.empty() );

	if( Line ) {
		out << Line->LineNumber << ":";
	}
#if 0
	switch( Severity ) {
		case ES_CriticalError:
			out << "critical error: ";
			break;
		case ES_Error:
			out << "error: ";
			break;
	}
#endif
	out << "error: " << Message;

	if( !Line ) {
		out << "." << endl;
	} else {
		const string& source = Line->Line;

		vector<bool> highlightsMask( source.length() + 1 );
		vector<bool> superHighlightsMask( source.length() + 1 );
		size_t lashHighlightOffset = 0;
		for( CLineSegment lineSegment : LineSegments ) {
			check_logic( lineSegment.Length > 0 );
			lineSegment.Length = min( lineSegment.Length, source.length() );
			const size_t first = min( lineSegment.Offset, source.length() );
			const size_t last = min( first + lineSegment.Length - 1, source.length() );
			lashHighlightOffset = max( lashHighlightOffset, last );
			for( size_t i = first; i <= last; i++ ) {
				highlightsMask[i] = true;
			}
			superHighlightsMask[last] = true;
		}
		string highlights;
		highlights.reserve( source.length() + 1 );

		for( size_t i = 0; i <= lashHighlightOffset; i++ ) {
			char c = i < source.length() ? source[i] : ' ';
			if( LsplTools::IsByteFirstInUtf8Symbol( c ) ) {
				if( superHighlightsMask[i] ) {
					highlights += '^';
				} else if( highlightsMask[i] ) {
					highlights += '~';
				} else {
					highlights += ' ';
				}
			}
		}
		out << ": " << endl << source << endl << highlights << endl << endl;
	}
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
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

} // end of Lspl namespace
