#include <common.h>
#include <Parser.h>
#include <Tokenizer.h>
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
		if( argc != 5 ) {
			cerr << "Usage: lspl2 CONFIGURATION PATTERNS TEXT RESULT" << endl;
			return 1;
		}

		CConfigurationPtr conf( new CConfiguration );
		if( !conf->LoadFromFile( argv[1], cout, cerr ) ) {
			return 1;
		}

		CAnnotation::SetArgreementBegin( conf->Attributes().Size() );
		for( TAttribute a = 0; a < conf->Attributes().Size(); a++ ) {
			if( conf->Attributes()[a].Agreement() ) {
				CAnnotation::SetArgreementBegin( a );
				break;
			}
		}

		CErrorProcessor errorProcessor;
		CPatternsBuilder patternsBuilder( conf, errorProcessor );
		patternsBuilder.ReadFromFile( argv[2] );
		patternsBuilder.CheckAndBuildIfPossible();

		if( errorProcessor.HasAnyErrors() ) {
			errorProcessor.PrintErrors( cerr, argv[2] );
			return 1;
		}

		const CPatterns patterns = patternsBuilder.GetResult();
		patterns.Print( cout );

		CText text( conf );
		if( !text.LoadFromFile( argv[3], cerr ) ) {
			return 1;
		}

		for( TReference ref = 0; ref < patterns.Size(); ref++ ) {
			const CPattern& pattern = patterns.Pattern( ref );
			cout << pattern.Name() << endl;

			CPatternBuildContext buildContext( patterns );
			CPatternVariants variants;
			pattern.Build( buildContext, variants, 12 );
			variants.Print( patterns, cout );
			variants.Build( buildContext );

			CMatchContext matchContext( text, buildContext.States );
			for( TWordIndex wi = 0; wi < text.Length(); wi++ ) {
				matchContext.Match( wi );
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
