#include <common.h>
#include <Configuration.h>

#define RAPIDJSON_ASSERT debug_check_logic
#include <rapidjson/rapidjson.h>
#include <rapidjson/schema.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/istreamwrapper.h>

namespace Lspl {
namespace Configuration {

///////////////////////////////////////////////////////////////////////////////

static const char* WordSignTypeString( const TWordSignType type )
{
	switch( type ) {
		case WST_None:
			break;
		case WST_Main:
			return "main";
		case WST_Enum:
			return "enum";
		case WST_String:
			return "string";
	}
	return "none";
}

void CWordSign::Print( ostream& out ) const
{
	out << ( Consistent ? "consistent " : "" )
		<< WordSignTypeString( Type )
		<< " word sign"
		<< endl;

	if( !Names.IsEmpty() ) {
		out << "  names: ";
		Names.Print( out, ", " );
		out << endl;
	}
	if( !Values.IsEmpty() ) {
		out << "  values: ";
		Values.Print( out, ", " );
		out << endl;
	}
}

CWordSigns::CWordSigns()
{
}

bool CWordSigns::IsEmpty() const
{
	return wordSigns.empty();
}

CWordSigns::SizeType CWordSigns::Size() const
{
	return wordSigns.size();
}

const CWordSign& CWordSigns::operator[]( SizeType& index ) const
{
	debug_check_logic( index < wordSigns.size() );
	return wordSigns[index];
}

bool CWordSigns::Find( const string& name, SizeType& index ) const
{
	auto i = nameIndices.find( name );
	if( i != nameIndices.cend() ) {
		index = i->second;
		return true;
	}
	return false;
}

void CWordSigns::Print( ostream& out ) const
{
	for( const CWordSign& wordSign : wordSigns ) {
		wordSign.Print( out );
		out << endl;
	}
}

CWordSignsBuilder::CWordSignsBuilder( const CWordSigns::SizeType count )
{
	mainSigns.reserve( count );
	consistentSigns.reserve( count );
	notConsistentSigns.reserve( count );
}

void CWordSignsBuilder::Add( CWordSign&& wordSign )
{
	if( wordSign.Type == WST_Main ) {
		mainSigns.push_back( wordSign );
	} else if( wordSign.Consistent ) {
		consistentSigns.push_back( wordSign );
	} else {
		notConsistentSigns.push_back( wordSign );
	}
}

bool CWordSignsBuilder::Build( ostream& errorStream, CWordSigns& dest )
{
	dest.wordSigns.clear();
	dest.nameIndices.clear();

	// MUST be exactly one main word sign
	// All word sign names MUST be unique
	bool success = true;

	if( mainSigns.size() != 1 ) {
		success = false;
		errorStream << "Configuration error: MUST be exactly one main word sign!" << endl;
	}

	vector<CWordSign> wordSigns = move( mainSigns );
	wordSigns.insert( wordSigns.end(),
		notConsistentSigns.cbegin(), notConsistentSigns.cend() );
	wordSigns.insert( wordSigns.end(),
		consistentSigns.cbegin(), consistentSigns.cend() );

	CWordSigns::CNameIndices nameIndices;
	for( CWordSigns::SizeType index = 0; index < wordSigns.size(); index++ ) {
		const CWordSign& wordSign = wordSigns[index];

		check_logic( wordSign.Type == WST_Enum
			|| wordSign.Type == WST_String
			|| wordSign.Type == WST_Main );
		check_logic( !wordSign.Names.IsEmpty() );
		check_logic( wordSign.Type != WST_String || wordSign.Values.IsEmpty() );
		check_logic( wordSign.Type != WST_Main || !wordSign.Consistent );

		for( COrderedStrings::SizeType i = 0; i < wordSign.Names.Size(); i++ ) {
			auto pair = nameIndices.insert(
				make_pair( wordSign.Names.Value( i ), index ) );
			if( !pair.second ) {
				success = false;
				errorStream << "Configuration error: redefinition of word sign name '"
					<< pair.first->first << "'!" << endl;
			}
		}
	}

	if( success ) {
		dest.wordSigns = move( wordSigns );
		dest.nameIndices = move( nameIndices );
	}
	return success;
}

static TWordSignType ParseWordSignType( const string& typeStr )
{
	if( typeStr == "enum" ) {
		return WST_Enum;
	} else if( typeStr == "string" ) {
		return WST_String;
	} else if( typeStr == "main" ) {
		return WST_Main;
	}
	return WST_None;
}

const char* JsonConfigurationSchemeText()
{
	static const char* const SchemeText = "\
{                                                                          \n\
  \"type\": \"object\",                                                    \n\
  \"properties\": {                                                        \n\
    \"word_signs\": {                                                      \n\
      \"type\": \"array\",                                                 \n\
      \"minItems\": 1,                                                     \n\
      \"items\": { \"$ref\": \"#/definitions/word_sign\" }                 \n\
    }                                                                      \n\
  },                                                                       \n\
  \"required\": [\"word_signs\"],                                          \n\
  \"additionalProperties\": false,                                         \n\
  \"definitions\": {                                                       \n\
    \"word_sign\": {                                                       \n\
      \"type\": \"object\",                                                \n\
      \"oneOf\": [                                                         \n\
        { \"$ref\": \"#/definitions/main_type\" },                         \n\
        { \"$ref\": \"#/definitions/enum_type\" },                         \n\
        { \"$ref\": \"#/definitions/string_type\" }                        \n\
      ]                                                                    \n\
    },                                                                     \n\
    \"main_type\": {                                                       \n\
      \"type\": \"object\",                                                \n\
      \"properties\": {                                                    \n\
        \"names\": { \"$ref\": \"#/definitions/string_array\" },           \n\
        \"values\": { \"$ref\": \"#/definitions/string_array\" },          \n\
        \"type\": {                                                        \n\
          \"type\": \"string\",                                            \n\
          \"pattern\": \"^main$\"                                          \n\
        }                                                                  \n\
      },                                                                   \n\
      \"required\": [\"names\", \"type\", \"values\"],                     \n\
      \"additionalProperties\": false                                      \n\
    },                                                                     \n\
    \"enum_type\": {                                                       \n\
      \"type\": \"object\",                                                \n\
      \"properties\": {                                                    \n\
        \"names\": { \"$ref\": \"#/definitions/string_array\" },           \n\
        \"values\": { \"$ref\": \"#/definitions/string_array\" },          \n\
        \"type\": {                                                        \n\
          \"type\": \"string\",                                            \n\
          \"pattern\": \"^enum$\"                                          \n\
        },                                                                 \n\
        \"consistent\": { \"type\": \"boolean\" }                          \n\
      },                                                                   \n\
      \"required\": [\"names\", \"type\", \"values\"],                     \n\
      \"additionalProperties\": false                                      \n\
    },                                                                     \n\
    \"string_type\": {                                                     \n\
      \"type\": \"object\",                                                \n\
      \"properties\": {                                                    \n\
        \"names\": { \"$ref\": \"#/definitions/string_array\" },           \n\
        \"type\": {                                                        \n\
          \"type\": \"string\",                                            \n\
          \"pattern\": \"^string$\"                                        \n\
        },                                                                 \n\
        \"consistent\": { \"type\": \"boolean\" }                          \n\
      },                                                                   \n\
      \"required\": [\"names\", \"type\"],                                 \n\
      \"additionalProperties\": false                                      \n\
    },                                                                     \n\
    \"string_array\": {                                                    \n\
      \"type\": \"array\",                                                 \n\
      \"minItems\": 1,                                                     \n\
      \"uniqueItems\": true,                                               \n\
      \"items\": {                                                         \n\
        \"type\": \"string\",                                              \n\
        \"pattern\": \"^[a-zA-Z]([a-zA-Z0-9_-]*[a-zA-Z_-])?$\"             \n\
      }                                                                    \n\
    }                                                                      \n\
  }                                                                        \n\
}                                                                          \n";
	return SchemeText;
}

class CConfigurationDerived : public CConfiguration {
public:
	using CConfiguration::wordSigns;

	CConfigurationDerived()
	{
	}
};

CConfigurationPtr LoadConfigurationFromFile( const char* filename,
	ostream& errorStream, ostream& logStream )
{
	using namespace rapidjson;

	logStream << "Loading configuration validation scheme..." << endl;

	Document schemeDocument;
	schemeDocument.Parse( JsonConfigurationSchemeText() );
	check_logic( !schemeDocument.HasParseError() );
	SchemaDocument scheme( schemeDocument );

	logStream << "Loading configuration from file '" << filename << "'..." << endl;

	ifstream ifs1( filename );
	IStreamWrapper isw1( ifs1 );

	Document configDocument;
	configDocument.ParseStream( isw1 );

	if( configDocument.HasParseError() ) {
		errorStream << "Parse config '" << filename << "' error at char "
			<< configDocument.GetErrorOffset() << ": "
			<< GetParseError_En( configDocument.GetParseError() ) << endl;
		return CConfigurationPtr();
	}

	logStream << "Validating configuration by scheme..." << endl;

	SchemaValidator schemeValidator( scheme );
	if( !configDocument.Accept( schemeValidator ) ) {
		StringBuffer sb;
		schemeValidator.GetInvalidSchemaPointer().StringifyUriFragment( sb );
		errorStream << "Invalid schema: " << sb.GetString() << endl;
		errorStream << "Invalid keyword: "
			<< schemeValidator.GetInvalidSchemaKeyword() << endl;
		sb.Clear();
		schemeValidator.GetInvalidDocumentPointer().StringifyUriFragment( sb );
		errorStream << "Invalid document: " << sb.GetString() << endl;
		return CConfigurationPtr();
	}

	logStream << "Building configuration..." << endl;

	Value wordSignArray = configDocument["word_signs"].GetArray();
	CWordSignsBuilder wordSignsBuilder( wordSignArray.Size() );
	for( rapidjson::SizeType i = 0; i < wordSignArray.Size(); i++ ) {
		CWordSign wordSign;
		Value wordSignObject = wordSignArray[i].GetObject();
		wordSign.Type = ParseWordSignType( wordSignObject["type"].GetString() );
		debug_check_logic( wordSign.Type != WST_None );

		Value nameArray = wordSignObject["names"].GetArray();
		for( rapidjson::SizeType ni = 0; ni < nameArray.Size(); ni++ ) {
			const bool added = wordSign.Names.Add( nameArray[ni].GetString() );
			debug_check_logic( added );
		}
		if( wordSignObject.HasMember( "values" ) ) {
			Value valueArray = wordSignObject["values"].GetArray();
			for( rapidjson::SizeType vi = 0; vi < valueArray.Size(); vi++ ) {
				const bool added = wordSign.Values.Add( valueArray[vi].GetString() );
				debug_check_logic( added );
			}
		}
		if( wordSignObject.HasMember( "consistent" ) ) {
			wordSign.Consistent = wordSignObject["consistent"].GetBool();
		}
		wordSignsBuilder.Add( move( wordSign ) );
	}

	unique_ptr<CConfigurationDerived> tmp( new CConfigurationDerived );
	if( !wordSignsBuilder.Build( errorStream, tmp->wordSigns ) ) {
		return CConfigurationPtr();
	}

	logStream << endl;
	CConfigurationPtr configuration( tmp.release() );
	configuration->WordSigns().Print( logStream );
	logStream << "Configuration was successfully built!" << endl << endl;
	return configuration;
}

///////////////////////////////////////////////////////////////////////////////

} // end of Configuration namespace
} // end of Lspl namespace
