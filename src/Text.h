#pragma once

#include <OrderedList.h>

///////////////////////////////////////////////////////////////////////////////
// basic_regex for CharEx

#pragma region

typedef uint32_t CharEx;

namespace std {
	template<>
	class regex_traits<CharEx> : public _Regex_traits_base {
	public:
		typedef CharEx _Uelem;
		typedef regex_traits<CharEx> _Myt;
		typedef CharEx char_type;
		typedef basic_string<CharEx> string_type;
		typedef locale locale_type;
		typedef typename string_type::size_type size_type;

		static size_type length( const CharEx* str )
		{
			return string_type( str ).length();
		}
		regex_traits()
		{
		}
		regex_traits( const regex_traits& _Right )
		{
		}
		regex_traits& operator=( const regex_traits& _Right )
		{
			return ( *this );
		}
		CharEx translate( CharEx c ) const
		{
			return c;
		}
		CharEx translate_nocase( CharEx c ) const
		{
			return c;
		}
		template<class Iter>
		string_type transform( Iter first, Iter last ) const
		{
			return string_type( first, last );
		}
		template<class Iter>
		string_type transform_primary( Iter first, Iter last ) const
		{
			return string_type( first, last );
		}
		template<class Iter>
		string_type lookup_collatename( Iter first, Iter last ) const
		{
			return string_type( first, last );
		}
		template<class Iter>
		char_class_type lookup_classname( Iter /*first*/, Iter /*last*/,
			bool /*case = false*/ ) const
		{
			return 0;
		}
		bool isctype( CharEx /*c*/, char_class_type /*type*/ ) const
		{
			return false;
		}
		int value( CharEx /*c*/, int /*base*/ ) const
		{
			throw logic_error( "regex_traits<CharEx>::value" );
			return -1;
		}
		locale_type imbue( locale_type /*newLocale*/ )
		{
			return locale();
		}
		locale_type getloc() const
		{
			return locale();
		}
	};
}

class StringEx : public basic_string<CharEx> {
public:
	StringEx()
	{
	}
	StringEx( const string& str )
	{
		AppendString( str );
	}
	StringEx( const basic_string<CharEx>& str ) :
		basic_string<CharEx>( str )
	{
	}
	StringEx& operator=( const string& str )
	{
		clear();
		AppendString( str );
		return *this;
	}
	StringEx& operator=( const basic_string<CharEx>& str )
	{
		basic_string<CharEx>::operator=( str );
		return *this;
	}

	void AppendString( const string& str )
	{
		reserve( length() + str.length() );
		for( const char c : str ) {
			push_back( static_cast<unsigned char>( c ) );
		}
	}

	string ToString() const
	{
		string result;
		result.reserve( length() );
		for( const CharEx c : *this ) {
			if( c < 128 ) {
				result.push_back( c );
			} else {
				throw logic_error( "StringEx::ToString()" );
			}
		}
		return result;
	}
};

template<>
struct hash<StringEx> {
	typedef StringEx argument_type;
	typedef size_t result_type;
	result_type operator()( argument_type const& str ) const
	{
		return hash<basic_string<CharEx>>{}( str );
	}
};

typedef basic_regex<CharEx> RegexEx;
typedef match_results<StringEx::const_iterator> MatchResultsEx;
#pragma endregion

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

enum TAgreementPower {
	AP_None,
	AP_Weak,
	AP_Strong
};

typedef StringEx CAttributes;
typedef CAttributes::size_type TAttributeIndex;

const TAttributeIndex MainAttribute = 0;
const CharEx AnyAttributeValue = static_cast<CharEx>( 128 );

class CAnnotation {
public:
	explicit CAnnotation( CAttributes&& attributes );

	bool Match( const RegexEx& attributesRegex ) const;
	TAgreementPower Agreement( const CAnnotation& annotation,
		const TAttributeIndex attribute = MainAttribute ) const;

	// sets interval for agreement as [index, attributes.size()]
	static void SetArgreementBegin( const TAttributeIndex index );

private:
	CAttributes attributes;

	static TAttributeIndex agreementBegin;
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
	bool MatchAttributes( const RegexEx& attributesRegex,
		CAnnotationIndices& indices ) const;
};

///////////////////////////////////////////////////////////////////////////////

typedef vector<CWord> CWords;
typedef CWords::size_type TWordIndex;

///////////////////////////////////////////////////////////////////////////////

typedef pair<CAnnotationIndices, CAnnotationIndices> CAgreement;

class CArgreements {
public:
	typedef pair<pair<TWordIndex, TWordIndex>, TAttributeIndex> CKey;

	explicit CArgreements( const CWords& text );
	const CAgreement& Agreement( const CKey& key, const bool strong ) const;

private:
	const CWords& words;
	typedef pair<CAgreement, CAgreement> CAgreementPair;
	mutable unordered_map<CKey, CAgreementPair> cache;
};

///////////////////////////////////////////////////////////////////////////////

class CText : public CWords {
public:
	CText() :
		argreements( static_cast<CWords&>( *this ) )
	{
	}

	bool HasWord( const TWordIndex wordIndex ) const
	{
		return ( wordIndex < size() );
	}

	const CArgreements& Argreements() const { return argreements; }

private:
	CArgreements argreements;
};

///////////////////////////////////////////////////////////////////////////////

} // end of Text namespace
} // end of Lspl namespace
