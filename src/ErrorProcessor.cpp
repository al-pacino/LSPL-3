#include <common.h>
#include <ErrorProcessor.h>

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
		out << ": " << endl << Line->Line << endl;

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
