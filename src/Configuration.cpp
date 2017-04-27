#include <common.h>
#include <Configuration.h>

#define RAPIDJSON_ASSERT debug_check_logic
#include <rapidjson/rapidjson.h>
#include <rapidjson/schema.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/istreamwrapper.h>

using namespace Lspl::Text;

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

///////////////////////////////////////////////////////////////////////////////

void CWordAttribute::Print( ostream& out ) const
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

///////////////////////////////////////////////////////////////////////////////

bool CWordAttributes::IsEmpty() const
{
	return data.empty();
}

Text::TAttribute CWordAttributes::Size() const
{
	return Cast<Text::TAttribute>( data.size() );
}

const CWordAttribute& CWordAttributes::Main() const
{
	debug_check_logic( !IsEmpty() );
	return data[Text::MainAttribute];
}

const CWordAttribute& CWordAttributes::operator[]( Text::TAttribute index ) const
{
	debug_check_logic( index < data.size() );
	return data[index];
}

bool CWordAttributes::Find( const string& name, Text::TAttribute& index ) const
{
	auto i = nameIndices.find( name );
	if( i != nameIndices.cend() ) {
		index = i->second;
		return true;
	}
	return false;
}

void CWordAttributes::Print( ostream& out ) const
{
	for( const CWordAttribute& wordAttribute : data ) {
		wordAttribute.Print( out );
		out << endl;
	}
}

///////////////////////////////////////////////////////////////////////////////

CWordAttributesBuilder::CWordAttributesBuilder( const Text::TAttribute count )
{
	mains.reserve( count );
	consistents.reserve( count );
	notConsistents.reserve( count );
}

void CWordAttributesBuilder::Add( CWordAttribute&& wordAttribute )
{
	if( wordAttribute.Type == WST_Main ) {
		mains.push_back( wordAttribute );
	} else if( wordAttribute.Consistent ) {
		consistents.push_back( wordAttribute );
	} else {
		notConsistents.push_back( wordAttribute );
	}
}

bool CWordAttributesBuilder::Build( ostream& errorStream, CWordAttributes& dest )
{
	// MUST be exactly one main word sign
	// All word sign names MUST be unique
	bool success = true;

	if( mains.size() != 1 ) {
		success = false;
		errorStream << "Configuration error: MUST be exactly one main word sign!" << endl;
	}

	vector<CWordAttribute> data = move( mains );
	data.insert( data.end(), notConsistents.cbegin(), notConsistents.cend() );
	data.insert( data.end(), consistents.cbegin(), consistents.cend() );

	CWordAttributes::CNameIndices nameIndices;
	for( Text::TAttribute index = 0; index < data.size(); index++ ) {
		const CWordAttribute& attribute = data[index];

		check_logic( attribute.Type == WST_Enum
			|| attribute.Type == WST_String
			|| attribute.Type == WST_Main );
		check_logic( !attribute.Names.IsEmpty() );
		check_logic( attribute.Type != WST_String || attribute.Values.IsEmpty() );
		check_logic( attribute.Type != WST_Main || !attribute.Consistent );

		for( COrderedStrings::SizeType i = 0; i < attribute.Names.Size(); i++ ) {
			auto pair = nameIndices.insert(
				make_pair( attribute.Names.Value( i ), index ) );
			if( !pair.second ) {
				success = false;
				errorStream
					<< "Configuration error: redefinition of word sign name '"
					<< pair.first->first << "'!" << endl;
			}
		}
	}

	if( success ) {
		dest.data = move( data );
		dest.nameIndices = move( nameIndices );
	}
	return success;
}

///////////////////////////////////////////////////////////////////////////////

void CConfiguration::SetAttributes( CWordAttributes&& attributes )
{
	wordAttributes = move( attributes );
}

///////////////////////////////////////////////////////////////////////////////

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
	CWordAttributesBuilder attributesBuilder( Cast<TAttribute>( wordSignArray.Size() ) );
	for( rapidjson::SizeType i = 0; i < wordSignArray.Size(); i++ ) {
		CWordAttribute attribute;
		Value wordSignObject = wordSignArray[i].GetObject();
		attribute.Type = ParseWordSignType( wordSignObject["type"].GetString() );
		debug_check_logic( attribute.Type != WST_None );

		Value nameArray = wordSignObject["names"].GetArray();
		for( rapidjson::SizeType ni = 0; ni < nameArray.Size(); ni++ ) {
			const bool added = attribute.Names.Add( nameArray[ni].GetString() );
			debug_check_logic( added );
		}
		if( wordSignObject.HasMember( "values" ) ) {
			if( attribute.Type == WST_Enum ) {
				attribute.Values.Add( "" );
			}
			Value valueArray = wordSignObject["values"].GetArray();
			for( rapidjson::SizeType vi = 0; vi < valueArray.Size(); vi++ ) {
				const bool added = attribute.Values.Add( valueArray[vi].GetString() );
				debug_check_logic( added );
			}
		}
		if( wordSignObject.HasMember( "consistent" ) ) {
			attribute.Consistent = wordSignObject["consistent"].GetBool();
		}
		attributesBuilder.Add( move( attribute ) );
	}

	CWordAttributes wordAttributes;
	if( !attributesBuilder.Build( errorStream, wordAttributes ) ) {
		return CConfigurationPtr();
	}

	logStream << endl;
	CConfigurationPtr configuration( new CConfiguration );
	configuration->SetAttributes( move( wordAttributes ) );
	configuration->Attributes().Print( logStream );
	logStream << "Configuration was successfully built!" << endl << endl;
	return configuration;
}

///////////////////////////////////////////////////////////////////////////////

} // end of Configuration namespace
} // end of Lspl namespace
