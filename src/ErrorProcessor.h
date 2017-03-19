#pragma once

#include <SharedFileLine.h>

namespace Lspl {
namespace Parser {

///////////////////////////////////////////////////////////////////////////////

struct CToken;

enum TErrorSeverity {
	ES_CriticalError,
	ES_Error
};

struct CError {
	TErrorSeverity Severity;
	CSharedFileLine Line;
	vector<CLineSegment> LineSegments;
	string Message;

	CError( CLineSegment lineSegments, CSharedFileLine line,
		const string& message = "", TErrorSeverity severity = ES_Error ) :
		Severity( severity ),
		Line( line ),
		Message( message )
	{
		LineSegments.push_back( lineSegments );
	}

	CError( const CToken& token, const string& message = "",
		TErrorSeverity severity = ES_Error );

	CError( CSharedFileLine line, const string& message,
			TErrorSeverity severity = ES_Error ) :
		Severity( severity ),
		Line( line ),
		Message( message )
	{
	}

	explicit CError( const string& message, TErrorSeverity severity = ES_Error ) :
		Severity( severity ),
		Message( message )
	{
	}

	void Print( ostream& out ) const;

private:
	string highlightSymbols() const;
};

///////////////////////////////////////////////////////////////////////////////

class CErrorProcessor {
	CErrorProcessor( const CErrorProcessor& ) = delete;
	CErrorProcessor& operator=( const CErrorProcessor& ) = delete;

public:
	CErrorProcessor();
	~CErrorProcessor();

	void Reset();
	void AddError( CError&& error );
	void PrintErrors( ostream& out, const string& filename = "" ) const;
	bool HasAnyErrors() const;
	bool HasCriticalErrors() const;

private:
	bool hasErrors;
	bool hasCriticalErrors;
	vector<vector<CError>> errors;
};

///////////////////////////////////////////////////////////////////////////////

} // end of Parser namespace
} // end of Lspl namespace
