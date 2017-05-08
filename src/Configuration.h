#pragma once

#include <OrderedList.h>
#include <Attributes.h>

namespace Lspl {
namespace Configuration {

///////////////////////////////////////////////////////////////////////////////

typedef size_t TDictionary;

///////////////////////////////////////////////////////////////////////////////

enum TWordAttributeType {
	WAT_Main,
	WAT_Enum,
	WAT_String
};

typedef uint8_t TAttributeNameIndex;
const TAttributeNameIndex MaxAttributeNameIndex
	= numeric_limits<TAttributeNameIndex>::max();

class CWordAttribute {
public:
	explicit CWordAttribute( const TWordAttributeType type,
		const bool agreement = false, const bool default = false );
	CWordAttribute( CWordAttribute&& ) = default;
	CWordAttribute& operator=( CWordAttribute&& ) = default;

	void AddName( const string& name );
	void AddValue( const string& value );

	const TWordAttributeType Type() const;
	const bool Agreement() const;
	const bool Default() const;
	const TAttributeNameIndex NamesCount() const;
	const string& Name( const TAttributeNameIndex index ) const;
	const Text::TAttributeValue ValuesCount() const;
	const string& Value( const Text::TAttributeValue index ) const;
	const bool FindValue( const string& value,
		Text::TAttributeValue& index ) const;
	void Print( ostream& out ) const;

private:
	TWordAttributeType type;
	bool agreement;
	bool default;
	vector<string> names;
	vector<string> values;
	typedef unordered_map<string, Text::TAttributeValue> CValueIndices;
	mutable CValueIndices valueIndices;
};

///////////////////////////////////////////////////////////////////////////////

class CWordAttributes {
	CWordAttributes( const CWordAttributes& ) = delete;
	CWordAttributes& operator=( const CWordAttributes& ) = delete;

public:
	CWordAttributes() = default;
	CWordAttributes( CWordAttributes&& ) = default;
	CWordAttributes& operator=( CWordAttributes&& ) = default;

	bool Initialize( vector<CWordAttribute>&& attributes, ostream& err );

	bool Valid() const;
	Text::TAttribute Size() const;
	const CWordAttribute& Main() const;
	const CWordAttribute& operator[]( Text::TAttribute index ) const;
	bool Find( const string& name, Text::TAttribute& index ) const;
	bool FindDefault( Text::TAttribute& index ) const;
	void Print( ostream& out ) const;

private:
	vector<CWordAttribute> data;
	Text::TAttribute defaultAttribute = Text::MainAttribute;
	typedef unordered_map<string, Text::TAttribute> CNameIndices;
	CNameIndices nameIndices;
};

///////////////////////////////////////////////////////////////////////////////

class CConfiguration {
	CConfiguration( const CConfiguration& ) = delete;
	CConfiguration& operator=( const CConfiguration& ) = delete;

public:
	CConfiguration() = default;

	const CWordAttributes& Attributes() const;
	bool LoadFromFile( const char* filename, ostream& out, ostream& err );

private:
	CWordAttributes wordAttributes;
};

typedef shared_ptr<CConfiguration> CConfigurationPtr;

///////////////////////////////////////////////////////////////////////////////

const char* JsonConfigurationSchemeText();

///////////////////////////////////////////////////////////////////////////////

} // end of Configuration namespace
} // end of Lspl namespace
