#pragma once

#include <OrderedList.h>
#include <Attributes.h>

///////////////////////////////////////////////////////////////////////////////

namespace std {
	template<typename FIRST_TYPE, typename SECOND_TYPE>
	struct hash<pair<FIRST_TYPE, SECOND_TYPE>> {
		typedef pair<FIRST_TYPE, SECOND_TYPE> argument_type;
		typedef size_t result_type;
		result_type operator()( const argument_type& pair ) const
		{
			return ( ( hash<FIRST_TYPE>{}( pair.first ) << 16 )
				^ hash<SECOND_TYPE>{}( pair.second ) );
		}
	};
}


///////////////////////////////////////////////////////////////////////////////

namespace Lspl {
namespace Text {

///////////////////////////////////////////////////////////////////////////////

typedef wstring StringEx;
typedef StringEx::value_type CharEx;
typedef basic_regex<CharEx> RegexEx;
typedef match_results<StringEx::const_iterator> MatchResultsEx;

StringEx ToStringEx( const string& str );
string FromStringEx( const StringEx& str );

///////////////////////////////////////////////////////////////////////////////

enum TAgreementPower {
	AP_None,
	AP_Weak,
	AP_Strong
};

class CAnnotation {
public:
	explicit CAnnotation( CAttributes&& attributes );

	const CAttributes& Attributes() const { return attributes; }
	TAgreementPower Agreement( const CAnnotation& annotation,
		const TAttribute attribute = MainAttribute ) const;

	// sets interval for agreement as [index, attributes.size()]
	static void SetArgreementBegin( const TAttribute attribute );

private:
	CAttributes attributes;

	static TAttribute agreementBegin;
};

typedef vector<CAnnotation> CAnnotations;
typedef CAnnotations::size_type TAnnotationIndex;
typedef COrderedList<TAnnotationIndex> CAnnotationIndices;

///////////////////////////////////////////////////////////////////////////////

struct CWord {
	string text;
	StringEx word;
	CAnnotations annotations;

	const CAnnotations& Annotations() const { return annotations; }
	CAnnotationIndices AnnotationIndices() const;
	bool MatchWord( const RegexEx& wordRegex ) const;
	bool MatchAttributes( const CAttributesRestriction& attributesRestriction,
		CAnnotationIndices& indices ) const;
};

///////////////////////////////////////////////////////////////////////////////

typedef vector<CWord> CWords;
typedef CWords::size_type TWordIndex;

///////////////////////////////////////////////////////////////////////////////

class CText {
	CText( const CText& ) = delete;
	CText& operator=( const CText& ) = delete;

public:
	explicit CText( CWords&& words );

	const CWord& Word( const TWordIndex index ) const;
	const TWordIndex Length() const { return words.size(); }

private:
	CWords words;
};

///////////////////////////////////////////////////////////////////////////////

} // end of Text namespace
} // end of Lspl namespace
