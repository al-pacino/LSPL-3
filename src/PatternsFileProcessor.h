#pragma once

#include <Tokenizer.h>

namespace LsplParser {

///////////////////////////////////////////////////////////////////////////////

class CPatternsFileProcessor {
	CPatternsFileProcessor( const CPatternsFileProcessor& ) = delete;
	CPatternsFileProcessor& operator=( const CPatternsFileProcessor& ) = delete;

public:
	explicit CPatternsFileProcessor( CErrorProcessor& errorProcessor );
	CPatternsFileProcessor( CErrorProcessor& errorProcessor, const string& filename );
	~CPatternsFileProcessor();

	// Opens the file filename
	void Open( const string& filename );
	// Check if a file is open
	bool IsOpen() const;
	// Closes opened file
	void Close();

	// Reads all tokens of the pattern which
	// starts from the current line of the file.
	// Allowed only if file is opened.
	void ReadPattern( CTokens& patternTokens );

private:
	CErrorProcessor& errorProcessor;
	ifstream file;
	CTokenizer tokenizer;
	size_t lineNumber;
	string line;

	void reset();
	void readLine();
	bool tokenizeLine();
	bool skipEmptyLines();
	bool lineStartsWithSpace() const;
};

///////////////////////////////////////////////////////////////////////////////

} // end of LsplParser namespace
