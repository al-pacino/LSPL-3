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

CWordAttribute::CWordAttribute( const TWordAttributeType _type,
		const bool _agreement, const bool _default ) :
	type( _type ),
	agreement( _agreement ),
	default( _default )
{
	if( type == WAT_Main ) {
		debug_check_logic( !agreement && !default );
	}

	if( type == WAT_Enum ) {
		AddValue( "null" );
	} else {
		AddValue( "" );
	}
}

void CWordAttribute::AddName( const string& name )
{
	debug_check_logic( find( names.cbegin(), names.cend(), name ) == names.cend() );
	names.push_back( name );
	debug_check_logic( names.size() <= MaxAttributeNameIndex );
}

void CWordAttribute::AddValue( const string& value )
{
	const TAttributeValue valueIndex = Cast<TAttributeValue>( values.size() );
	const auto pair = valueIndices.insert( make_pair( value, valueIndex ) );
	debug_check_logic( pair.second );
	values.push_back( value );
}

const TWordAttributeType CWordAttribute::Type() const
{
	return type;
}

const bool CWordAttribute::Agreement() const
{
	return agreement;
}

const bool CWordAttribute::Default() const
{
	return default;
}

const TAttributeNameIndex CWordAttribute::NamesCount() const
{
	return Cast<TAttributeNameIndex>( names.size() );
}

const string& CWordAttribute::Name( const TAttributeNameIndex index ) const
{
	debug_check_logic( index < names.size() );
	return names[index];
}

const TAttributeValue CWordAttribute::ValuesCount() const
{
	return Cast<TAttributeValue>( values.size() );
}

const string& CWordAttribute::Value( const TAttributeValue index ) const
{
	debug_check_logic( index < values.size() );
	return values[index];
}

const bool CWordAttribute::FindValue( const string& value,
	TAttributeValue& index ) const
{
	CValueIndices::const_iterator valueIterator;

	if( type == WAT_String ) {
		const TAttributeValue valueIndex = Cast<TAttributeValue>( values.size() );
		const auto pair = valueIndices.insert( make_pair( value, valueIndex ) );
		valueIterator = pair.first;
	} else {
		valueIterator = valueIndices.find( value );
	}

	if( valueIterator != valueIndices.cend() ) {
		index = valueIterator->second;
		return true;
	}
	return false;
}

void CWordAttribute::Print( ostream& out ) const
{
	if( agreement ) {
		out << "agreement ";
	}

	if( default ) {
		out << "default ";
	}

	switch( type ) {
		case WAT_Main:
			out << "main ";
			break;
		case WAT_Enum:
			out << "enum ";
			break;
		case WAT_String:
			out << "string ";
			break;
	}

	out << "word attribute" << endl;

	{
		debug_check_logic( !names.empty() );
		out << "  names: ";
		for( TAttributeNameIndex i = 0; i < NamesCount(); i++ ) {
			if( i > 0 ) {
				out << ", ";
			}
			out << Name( i );
		}
		out << endl;
	}

	if( type != WAT_String ) {
		out << "  values: ";
		for( TAttributeValue i = 1; i < ValuesCount(); i++ ) {
			if( i > 1 ) {
				out << ", ";
			}
			out << Value( i );
		}
		out << endl;
	}
}

///////////////////////////////////////////////////////////////////////////////

bool CWordAttributes::Initialize( vector<CWordAttribute>&& attributes,
	ostream& err )
{
	bool success = true;

	if( MaxAttribute < attributes.size() ) {
		success = false;
		err << "Configuration error: "
			<< "more than 255 word attributes are not allowed!";
	}

	bool hasMain = false;
	vector<CWordAttribute> tmp;
	tmp.reserve( attributes.size() );
	for( CWordAttribute& attribute : attributes ) {
		if( attribute.Type() == WAT_Main ) {
			if( hasMain ) {
				success = false;
				err << "Configuration error: "
					<< "must be exactly one main word attribute!";
			} else {
				hasMain = true;
				tmp.emplace( tmp.begin(), move( attribute ) );
			}
		} else if( attribute.Agreement() ) {
			if( hasMain ) {
				tmp.emplace( next( tmp.begin() ), move( attribute ) );
			} else {
				tmp.emplace( tmp.begin(), move( attribute ) );
			}
		} else {
			tmp.emplace_back( move( attribute ) );
		}
	}
	attributes.clear();

	TAttribute attributeIndex = 0;
	CNameIndices tmpIndices;
	for( const CWordAttribute& attribute : tmp ) {
		debug_check_logic( attribute.NamesCount() > 0 );
		for( TAttributeNameIndex i = 0; i < attribute.NamesCount(); i++ ) {
			const auto pair = tmpIndices.insert(
				make_pair( attribute.Name( i ), attributeIndex ) );
			if( !pair.second ) {
				success = false;
				err << "Configuration error: "
					<< "redefinition of word attribute name '"
					<< pair.first->first << "'!" << endl;
			}
		}
		if( attribute.Default() ) {
			if( defaultAttribute == MainAttribute ) {
				defaultAttribute = attributeIndex;
			} else {
				err << "Configuration error: "
					<< "more than one default attribute is not allowed" << endl;
			}
		}
		attributeIndex++;
	}

	if( success ) {
		data = move( tmp );
		nameIndices = move( tmpIndices );
	}
	return success;
}

bool CWordAttributes::Valid( ) const
{
	return !data.empty();
}

TAttribute CWordAttributes::Size() const
{
	return Cast<Text::TAttribute>( data.size() );
}

const CWordAttribute& CWordAttributes::Main() const
{
	debug_check_logic( Valid() );
	return data[Text::MainAttribute];
}

const CWordAttribute& CWordAttributes::operator[]( TAttribute index ) const
{
	debug_check_logic( Valid() );
	debug_check_logic( index < data.size() );
	return data[index];
}

bool CWordAttributes::Find( const string& name, TAttribute& index ) const
{
	debug_check_logic( Valid() );
	auto i = nameIndices.find( name );
	if( i != nameIndices.cend() ) {
		index = i->second;
		return true;
	}
	return false;
}

bool CWordAttributes::FindDefault( TAttribute& index ) const
{
	debug_check_logic( Valid() );
	if( defaultAttribute != MainAttribute ) {
		index = defaultAttribute;
		return true;
	}
	return false;
}

void CWordAttributes::Print( ostream& out ) const
{
	debug_check_logic( Valid() );
	for( const CWordAttribute& wordAttribute : data ) {
		wordAttribute.Print( out );
		out << endl;
	}
}

///////////////////////////////////////////////////////////////////////////////

const CWordAttributes& CConfiguration::Attributes() const
{
	return wordAttributes;
}

bool CConfiguration::LoadFromFile( const char* filename,
	ostream& out, ostream& err )
{
	using namespace rapidjson;

	out << "Loading configuration validation scheme..." << endl;

	Document schemeDocument;
	schemeDocument.Parse( JsonConfigurationSchemeText() );
	check_logic( !schemeDocument.HasParseError() );
	SchemaDocument scheme( schemeDocument );

	out << "Loading configuration from file '" << filename << "'..." << endl;

	ifstream ifs1( filename );
	IStreamWrapper isw1( ifs1 );

	Document configDocument;
	configDocument.ParseStream( isw1 );

	if( configDocument.HasParseError() ) {
		err << "Parse config '" << filename << "' error at char "
			<< configDocument.GetErrorOffset() << ": "
			<< GetParseError_En( configDocument.GetParseError() ) << endl;
		return false;
	}

	out << "Validating configuration by scheme..." << endl;

	SchemaValidator schemeValidator( scheme );
	if( !configDocument.Accept( schemeValidator ) ) {
		StringBuffer sb;
		schemeValidator.GetInvalidSchemaPointer().StringifyUriFragment( sb );
		err << "Invalid schema: " << sb.GetString() << endl;
		err << "Invalid keyword: "
			<< schemeValidator.GetInvalidSchemaKeyword() << endl;
		sb.Clear();
		schemeValidator.GetInvalidDocumentPointer().StringifyUriFragment( sb );
		err << "Invalid document: " << sb.GetString() << endl;
		return false;
	}

	out << "Building configuration..." << endl;

	Value wordSignArray = configDocument["word_signs"].GetArray();
	vector<CWordAttribute> attributes;
	attributes.reserve( wordSignArray.Size() );

	for( rapidjson::SizeType i = 0; i < wordSignArray.Size(); i++ ) {
		Value wordSignObject = wordSignArray[i].GetObject();

		TWordAttributeType type = WAT_Main;
		{
			const string typeString = wordSignObject["type"].GetString();
			if( typeString == "enum" ) {
				type = WAT_Enum;
			} else if( typeString == "string" ) {
				type = WAT_String;
			} else {
				debug_check_logic( typeString == "main" );
			}
		}

		bool agreement = false;
		if( wordSignObject.HasMember( "consistent" ) ) {
			agreement = wordSignObject["consistent"].GetBool();
		}

		bool default = false;
		if( wordSignObject.HasMember( "default" ) ) {
			default = wordSignObject["default"].GetBool();
		}

		attributes.emplace_back( type, agreement, default );

		Value nameArray = wordSignObject["names"].GetArray();
		for( rapidjson::SizeType ni = 0; ni < nameArray.Size(); ni++ ) {
			attributes.back().AddName( nameArray[ni].GetString() );
		}

		if( wordSignObject.HasMember( "values" ) ) {
			Value valueArray = wordSignObject["values"].GetArray();
			for( rapidjson::SizeType vi = 0; vi < valueArray.Size(); vi++ ) {
				attributes.back().AddValue( valueArray[vi].GetString() );
			}
		}
	}

	if( !wordAttributes.Initialize( move( attributes ), err ) ) {
		return false;
	}

	debug_check_logic( Attributes()[0].Type() == WAT_Main );

	out << endl;
	Attributes().Print( out );
	out << "Configuration was successfully initialized!" << endl << endl;

	return true;
}

///////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////

} // end of Configuration namespace
} // end of Lspl namespace
