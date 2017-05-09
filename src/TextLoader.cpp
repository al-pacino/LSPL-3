#include <common.h>
#include <Text.h>

#define RAPIDJSON_ASSERT debug_check_logic
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/istreamwrapper.h>

using namespace rapidjson;
using namespace Lspl::Configuration;

///////////////////////////////////////////////////////////////////////////////

namespace Lspl {
namespace Text {

///////////////////////////////////////////////////////////////////////////////

bool CText::LoadFromFile( const string& filename, ostream& err )
{
	words.clear();

	Document textDocument;
	{
		ifstream ifs1( filename );
		IStreamWrapper isw1( ifs1 );
		textDocument.ParseStream( isw1 );
	}
	if( textDocument.HasParseError() ) {
		err << "Parse text '" << filename << "' error at char "
			<< textDocument.GetErrorOffset() << ": "
			<< GetParseError_En( textDocument.GetParseError() ) << endl;
		return false;
	}

	if( !textDocument.IsObject()
		|| !textDocument.HasMember( "text" )
		|| !textDocument["text"].IsArray() )
	{
		err << "bad 'text' element" << endl;
		return false;
	}

	const CWordAttributes& wordAttributes = configuration->Attributes();

	CWords tempWords;
	Value wordsArray = textDocument["text"].GetArray();
	tempWords.reserve( wordsArray.Size() );
	for( rapidjson::SizeType wi = 0; wi < wordsArray.Size(); wi++ ) {
		if( !wordsArray[wi].IsObject()
			|| !wordsArray[wi].HasMember( "word" )
			|| !wordsArray[wi]["word"].IsString()
			|| !wordsArray[wi].HasMember( "annotations" )
			|| !wordsArray[wi]["annotations"].IsArray()
			|| wordsArray[wi]["annotations"].GetArray().Size() == 0 )
		{
			err << "bad 'word' #" << wi << " element" << endl;
			return false;
		}

		Value wordObject = wordsArray[wi].GetObject();

		tempWords.emplace_back();
		tempWords.back().text = wordObject["word"].GetString();
		tempWords.back().word = ToStringEx( tempWords.back().text );

		Value annotations = wordObject["annotations"].GetArray();
		if( MaxAnnotation < annotations.Size() ) {
			err << "bad 'word' #" << wi
				<< " too much annotations" << endl;
			return false;
		}
		tempWords.back().annotations.reserve( annotations.Size() );

		for( rapidjson::SizeType ai = 0; ai < annotations.Size(); ai++ ) {
			if( !annotations[ai].IsObject() ) {
				err << "bad 'word' #" << wi
					<< " 'annotation' #" << ai
					<< " element" << endl;
				return false;
			}

			CAttributes attributes( wordAttributes.Size() );

			Value attrObject = annotations[ai].GetObject();
			for( Value::ConstMemberIterator attr = attrObject.MemberBegin();
				attr != attrObject.MemberEnd(); ++attr )
			{
				if( !attr->value.IsString() ) {
					err << "bad 'word' #" << wi
						<< " 'annotation' #" << ai
						<< " attribute value" << endl;
					return false;
				}

				TAttribute index;
				if( wordAttributes.Find( attr->name.GetString(), index ) ) {
					const CWordAttribute& attribute = wordAttributes[index];
					const string value = attr->value.GetString();
					TAttributeValue attrValue = NullAttributeValue;
					if( attribute.FindValue( value, attrValue ) ) {
						if( attributes.Get( index ) == NullAttributeValue ) {
							attributes.Set( index, attrValue );
						} else {
							err << "bad 'word' #" << wi
								<< " 'annotation' #" << ai
								<< " redefinition of value" << endl;
							return false;
						}
					}
				}
			}

			if( attributes.Get( MainAttribute ) == NullAttributeValue ) {
				err << "bad 'word' #" << wi
					<< " 'annotation' #" << ai
					<< " has no main attribute" << endl;
				return false;
			}

			tempWords.back().annotations.emplace_back( move( attributes ) );
		}
	}

	words = move( tempWords );
	return true;
}

///////////////////////////////////////////////////////////////////////////////

} // end of Text namespace
} // end of Lspl namespace
