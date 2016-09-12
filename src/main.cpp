#include <common.h>

#include <Parser.h>
#include <Tokenizer.h>
#include <ErrorProcessor.h>
#include <PatternsFileProcessor.h>

using namespace LsplParser;

int main( int argc, const char* argv[] )
{
	try {
		if( argc != 2 ) {
			cerr << "Usage: lspl2 FILE" << endl;
			return 1;
		}

		CErrorProcessor errorProcessor;
		CPatternsFileProcessor reader( errorProcessor, argv[1] );

		CTokens tokens;
		while( reader.IsOpen() && !errorProcessor.HasCriticalErrors() ) {
			reader.ReadPattern( tokens );
			tokens.Print( cout );
			cout << endl;
		}

		if( errorProcessor.HasAnyErrors() ) {
			errorProcessor.PrintErrors( cerr, argv[1] );
			return 1;
		}
		// todo:
	} catch( exception& e ) {
		cerr << e.what() << endl;
		return 1;
	} catch( ... ) {
		cerr << "unknown error!";
		return 1;
	}
	return 0;
}
