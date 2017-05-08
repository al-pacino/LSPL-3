#include <common.h>
#include <Parser.h>
#include <Tokenizer.h>
#include <TextLoader.h>
#include <PatternMatch.h>
#include <Configuration.h>
#include <ErrorProcessor.h>
#include <PatternsFileProcessor.h>

using namespace Lspl;
using namespace Lspl::Text;
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

		CConfigurationPtr conf( new CConfiguration );
		if( !conf->LoadFromFile( argv[1], cout, cerr ) ) {
			return 1;
		}

		for( TAttribute a = 0; a < conf->Attributes().Size(); a++ ) {
			if( conf->Attributes()[a].Agreement() ) {
				CAnnotation::SetArgreementBegin( a );
				break;
			}
		}

		CErrorProcessor errorProcessor;
		CPatternsBuilder patternsBuilder( conf, errorProcessor );
		patternsBuilder.Read( argv[2] );
		patternsBuilder.Check();

		if( errorProcessor.HasAnyErrors() ) {
			errorProcessor.PrintErrors( cerr, argv[2] );
			return 1;
		}

		const CPatterns patterns = patternsBuilder.Save();
		patterns.Print( cout );

		CWords words;
		LoadText( patterns, argv[3], words );
		CText text( move( words ) );

		for( TReference ref = 0; ref < patterns.Size(); ref++ ) {
			const CPattern& pattern = patterns.Pattern( ref );
			cout << pattern.Name() << endl;
			CStates states;
			{
				CPatternBuildContext buildContext( patterns );
				CPatternVariants variants;
				pattern.Build( buildContext, variants, 12 );
				variants.Print( patterns, cout );
				states = move( variants.Build( patterns ) );
			}
			{
				CMatchContext matchContext( text, states );
				for( TWordIndex wi = 0; wi < text.Length(); wi++ ) {
					matchContext.Match( wi );
				}
			}
			cout << endl;
		}
	} catch( exception& e ) {
		cerr << e.what() << endl;
		return 1;
	} catch( ... ) {
		cerr << "unknown error!";
		return 1;
	}
	return 0;
}
