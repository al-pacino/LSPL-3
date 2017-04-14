#include <common.h>
#include <Parser.h>
#include <Tokenizer.h>
#include <Configuration.h>
#include <ErrorProcessor.h>
#include <PatternsFileProcessor.h>

using namespace Lspl::Parser;
using namespace Lspl::Pattern;
using namespace Lspl::Configuration;

int main( int argc, const char* argv[] )
{
	try {
		if( argc != 4 ) {
			cerr << "Usage: lspl2 CONFIGURATION PATTERNS TEXT" << endl;
			return 1;
		}

		CConfigurationPtr conf = LoadConfigurationFromFile( argv[1], cerr, cout );

		if( !static_cast<bool>( conf ) ) {
			return 1;
		}

		CErrorProcessor errorProcessor;
		CPatternsBuilder patternsBuilder( conf, errorProcessor );
		patternsBuilder.Read( argv[2] );
		patternsBuilder.Check();

		if( errorProcessor.HasAnyErrors() ) {
			errorProcessor.PrintErrors( cerr, argv[2] );
			return 1;
		}

		CPatterns patterns( patternsBuilder.Save() );
	} catch( exception& e ) {
		cerr << e.what() << endl;
		return 1;
	} catch( ... ) {
		cerr << "unknown error!";
		return 1;
	}
	return 0;
}
