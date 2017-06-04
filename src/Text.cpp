#include <common.h>
#include <Text.h>

///////////////////////////////////////////////////////////////////////////////

namespace Lspl {
namespace Text {

///////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
StringEx ToStringEx( const string& str )
{
	wstring_convert<codecvt_utf8<CharEx>, CharEx> cvt;
	return cvt.from_bytes( str );
}

string FromStringEx( const StringEx& str )
{
	wstring_convert<codecvt_utf8<CharEx>, CharEx> cvt;
	return cvt.to_bytes( str );
}
#else
StringEx ToStringEx( const string& utf8 )
{
	vector<unsigned long> unicode;
	size_t i = 0;
	while( i < utf8.size() ) {
		unsigned long uni;
		size_t todo;
		unsigned char ch = utf8[i++];
		if( ch <= 0x7F ) {
			uni = ch;
			todo = 0;
		} else if( ch <= 0xBF ) {
			throw logic_error( "not a UTF-8 string" );
		} else if( ch <= 0xDF ) {
			uni = ch & 0x1F;
			todo = 1;
		} else if( ch <= 0xEF ) {
			uni = ch & 0x0F;
			todo = 2;
		} else if( ch <= 0xF7 ) {
			uni = ch & 0x07;
			todo = 3;
		} else {
			throw logic_error( "not a UTF-8 string" );
		}
		for( size_t j = 0; j < todo; ++j ) {
			if( i == utf8.size() )
				throw logic_error( "not a UTF-8 string" );
			ch = utf8[i++];
			if( ch < 0x80 || ch > 0xBF )
				throw logic_error( "not a UTF-8 string" );
			uni <<= 6;
			uni += ch & 0x3F;
		}
		if( uni >= 0xD800 && uni <= 0xDFFF )
			throw logic_error( "not a UTF-8 string" );
		if( uni > 0x10FFFF )
			throw logic_error( "not a UTF-8 string" );
		unicode.push_back( uni );
	}
	StringEx utf16;
	for( i = 0; i < unicode.size(); ++i ) {
		unsigned long uni = unicode[i];
		if( uni <= 0xFFFF ) {
			utf16 += (wchar_t)uni;
		} else {
			uni -= 0x10000;
			utf16 += (wchar_t)( ( uni >> 10 ) + 0xD800 );
			utf16 += (wchar_t)( ( uni & 0x3FF ) + 0xDC00 );
		}
	}
	return utf16;
}

string FromStringEx( const StringEx& /*str*/ )
{
	throw logic_error( "unsupported" );
}
#endif

///////////////////////////////////////////////////////////////////////////////

CAnnotation::CAnnotation( CAttributes&& _attributes ) :
	attributes( _attributes )
{
	debug_check_logic( attributes.Get( MainAttribute ) != NullAttributeValue );
}

TAgreementPower CAnnotation::Agreement( const CAnnotation& annotation,
	const TAttribute attribute ) const
{
	debug_check_logic( MainAttribute < agreementBegin );
	debug_check_logic( attributes.Size() == annotation.attributes.Size() );
	debug_check_logic( attribute == MainAttribute || agreementBegin <= attribute );

	const TAttribute begin =
		( attribute != MainAttribute ? attribute : agreementBegin );
	const TAttribute end =
		( attribute != MainAttribute ? attribute : attributes.Size() );

	TAgreementPower power = AP_Strong;
	for( TAttribute i = begin; i < end; i++ ) {
		const TAttributeValue av1 = attributes.Get( i );
		const TAttributeValue av2 = annotation.attributes.Get( i );
		if( av1 != av2 ) {
			if( av1 == NullAttributeValue || av2 == NullAttributeValue ) {
				power = AP_Weak;
			} else {
				return AP_None;
			}
		}
	}
	return power;
}

TAttribute CAnnotation::agreementBegin = MainAttribute;

void CAnnotation::SetArgreementBegin( const TAttribute attribute )
{
	agreementBegin = attribute;
}

///////////////////////////////////////////////////////////////////////////////

CAnnotationIndices CWord::AnnotationIndices() const
{
	CAnnotationIndices indices;
	for( TAnnotationIndex i = 0; i < annotations.size(); i++ ) {
		indices.Add( i );
	}
	return indices;
}

bool CWord::MatchWord( const RegexEx& wordRegex ) const
{
	MatchResultsEx match;
	return regex_match( word, match, wordRegex );
}

bool CWord::MatchAttributes( const CAttributesRestriction& attributesRestriction,
	CAnnotationIndices& indices ) const
{
	indices.Empty();
	for( TAnnotationIndex i = 0; i < annotations.size(); i++ ) {
		if( attributesRestriction.Check( annotations[i].Attributes() ) ) {
			indices.Add( i );
		}
	}
	return !indices.IsEmpty();
}

///////////////////////////////////////////////////////////////////////////////

CText::CText( const Configuration::CConfigurationPtr _configuration ) :
	configuration( _configuration )
{
	check_logic( static_cast<bool>( configuration ) );
}

const CWord& CText::Word( const TWordIndex index ) const
{
	debug_check_logic( index < words.size() );
	return words[index];
}

///////////////////////////////////////////////////////////////////////////////

} // end of Text namespace
} // end of Lspl namespace
