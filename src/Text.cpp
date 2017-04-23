#include <common.h>
#include <Text.h>

///////////////////////////////////////////////////////////////////////////////

namespace Lspl {
namespace Text {

///////////////////////////////////////////////////////////////////////////////

bool CAnnotation::Match( const RegexEx& attributesRegex ) const
{
	MatchResultsEx match;
	return regex_match( Attributes, match, attributesRegex );
}

TAgreementPower CAnnotation::Agreement( const CAnnotation& annotation,
	const TAttributeIndex attribute ) const
{
	debug_check_logic( Attributes.length() == annotation.Attributes.length() );
	// TODO: checks

	const TAttributeIndex begin =
		( attribute != MainAttribute ? attribute : 1 );
	const TAttributeIndex end =
		( attribute != MainAttribute ? attribute : Attributes.length() );

	TAgreementPower power = AP_Strong;
	for( TAttributeIndex i = begin; i < end; i++ ) {
		const CharEx c1 = Attributes[i];
		const CharEx c2 = annotation.Attributes[i];
		if( c1 != c2 ) {
			if( c1 == ConformAny || c2 == ConformAny ) {
				power = AP_Weak;
			} else {
				return AP_None;
			}
		}
	}
	return power;
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

bool CWord::MatchAttributes( const RegexEx& attributesRegex,
	CAnnotationIndices& indices ) const
{
	debug_check_logic( indices.IsEmpty() );
	for( TAnnotationIndex i = 0; i < annotations.size(); i++ ) {
		if( annotations[i].Match( attributesRegex ) ) {
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

} // end of Text namespace
} // end of Lspl namespace
