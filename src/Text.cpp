#include <common.h>
#include <Text.h>

///////////////////////////////////////////////////////////////////////////////

namespace Lspl {
namespace Text {

///////////////////////////////////////////////////////////////////////////////

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

CText::CText( CWords&& _words ) :
	words( move( _words ) )
{
}

const CWord& CText::Word( const TWordIndex index ) const
{
	debug_check_logic( index < words.size() );
	return words[index];
}

///////////////////////////////////////////////////////////////////////////////

} // end of Text namespace
} // end of Lspl namespace
