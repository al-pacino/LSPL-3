#include <common.h>
#include <Parser.h>
#include <Tokenizer.h>
#include <Configuration.h>
#include <ErrorProcessor.h>
#include <PatternsFileProcessor.h>

using namespace Lspl::Parser;
using namespace Lspl::Configuration;

void ReadPatternDefinitions( const char* filename,
	CErrorProcessor& errorProcessor,
	/* out */ vector<CPatternDefinitionPtr>& patternDefs )
{
	patternDefs.clear();
	CPatternsFileProcessor reader( errorProcessor, filename );
	CTokens tokens;
	CPatternParser parser( errorProcessor );
	while( reader.IsOpen() && !errorProcessor.HasCriticalErrors() ) {
		reader.ReadPattern( tokens );
		patternDefs.push_back( parser.Parse( tokens ) );
		if( !static_cast<bool>( patternDefs.back() ) ) {
			patternDefs.pop_back();
		}
	}
}

int main( int argc, const char* argv[] )
{
	try {
		if( argc != 4 ) {
			cerr << "Usage: lspl2 CONFIGURATION PATTERNS TEXT" << endl;
			return 1;
		}

		CConfigurationPtr configuration =
			LoadConfigurationFromFile( argv[1], cerr, cout );

		if( !static_cast<bool>( configuration ) ) {
			return 1;
		}

		CErrorProcessor errorProcessor;
		unordered_map<string, CPatternDefinitionPtr> namePatternDefs;

		vector<CPatternDefinitionPtr> patternDefs;
		ReadPatternDefinitions( argv[2], errorProcessor, patternDefs );
		if( !errorProcessor.HasCriticalErrors() ) {
			vector<CTokenPtr> references;
			for( CPatternDefinitionPtr& patternDef : patternDefs ) {
				const string patternName = patternDef->Check( *configuration,
					errorProcessor, references );
				if( errorProcessor.HasCriticalErrors() ) {
					break;
				}
				if( patternName.empty() ) {
					continue;
				}
				const CTokenPtr nameToken = patternDef->Name;
				auto pair = namePatternDefs.insert(
					make_pair( patternName, move( patternDef ) ) );
				if( !pair.second ) {
					errorProcessor.AddError( CError( *nameToken,
						"pattern with such name was already defined" ) );
				}
			}
		}

		if( errorProcessor.HasAnyErrors() ) {
			errorProcessor.PrintErrors( cerr, argv[2] );
			return 1;
		} else {
			for( const auto& pattern : namePatternDefs ) {
				cout << pattern.first << endl;
			}
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
