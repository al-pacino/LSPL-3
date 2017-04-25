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
		const CharEx c1 = attributes.Get( i );
		const CharEx c2 = annotation.attributes.Get( i );
		if( c1 != c2 ) {
			if( c1 == NullAttributeValue || c2 == NullAttributeValue ) {
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

CArgreements::CArgreements( const CWords& text ) :
	words( text )
{
}

const CAgreement& CArgreements::Agreement( const CKey& key, const bool strong ) const
{
	debug_check_logic( key.first.first < key.first.second );
	debug_check_logic( key.first.second < words.size() );
	auto pair = cache.insert( make_pair( key, CAgreementPair() ) );
	CAgreementPair& agreementsPair = pair.first->second;
	if( pair.second ) {
		const CAnnotations& annotations1 = words[key.first.first].Annotations();
		const CAnnotations& annotations2 = words[key.first.second].Annotations();

		for( TAnnotationIndex i1 = 0; i1 < annotations1.size(); i1++ ) {
			for( TAnnotationIndex i2 = 0; i2 < annotations2.size(); i2++ ) {
				switch( annotations1[i1].Agreement( annotations2[i2], key.second ) ) {
					case AP_None:
						break;
					case AP_Strong:
						agreementsPair.first.first.Add( i1 );
						agreementsPair.first.second.Add( i2 );
						// break missed correctly
					case AP_Weak:
						agreementsPair.second.first.Add( i1 );
						agreementsPair.second.second.Add( i2 );
						break;
				}
			}
		}
	}
	return ( strong ? agreementsPair.first : agreementsPair.second );
}

///////////////////////////////////////////////////////////////////////////////

CText::CText( CWords&& _words ) :
	words( move( _words ) ),
	argreements( words )
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
