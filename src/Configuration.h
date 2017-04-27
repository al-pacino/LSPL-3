#pragma once

#include <OrderedList.h>
#include <Attributes.h>

namespace Lspl {
namespace Configuration {

///////////////////////////////////////////////////////////////////////////////

typedef size_t TDictionary;

///////////////////////////////////////////////////////////////////////////////

enum TWordSignType {
	WST_None,
	WST_Main,
	WST_Enum,
	WST_String
};

typedef COrderedList<string> COrderedStrings;

struct CWordAttribute {
	bool Consistent;
	TWordSignType Type;
	COrderedStrings Names;
	COrderedStrings Values;

	CWordAttribute() :
		Consistent( false ),
		Type( WST_None )
	{
	}

	void Print( ostream& out ) const;
};

///////////////////////////////////////////////////////////////////////////////

class CWordAttributes {
	friend class CWordAttributesBuilder;
	CWordAttributes( const CWordAttributes& ) = delete;
	CWordAttributes& operator=( const CWordAttributes& ) = delete;

public:
	CWordAttributes() = default;
	CWordAttributes( CWordAttributes&& ) = default;
	CWordAttributes& operator=( CWordAttributes&& ) = default;
	bool IsEmpty() const;
	Text::TAttribute Size() const;
	const CWordAttribute& Main() const;
	const CWordAttribute& operator[]( Text::TAttribute index ) const;
	bool Find( const string& name, Text::TAttribute& index ) const;
	void Print( ostream& out ) const;

private:
	vector<CWordAttribute> data;
	typedef unordered_map<string, Text::TAttribute> CNameIndices;
	CNameIndices nameIndices;
};

///////////////////////////////////////////////////////////////////////////////

class CWordAttributesBuilder {
public:
	explicit CWordAttributesBuilder( const Text::TAttribute size );
	void Add( CWordAttribute&& wordAttribute );
	bool Build( ostream& errorStream, CWordAttributes& wordSigns );

private:
	vector<CWordAttribute> mains;
	vector<CWordAttribute> consistents;
	vector<CWordAttribute> notConsistents;
};

///////////////////////////////////////////////////////////////////////////////

class CConfiguration {
	CConfiguration( const CConfiguration& ) = delete;
	CConfiguration& operator=( const CConfiguration& ) = delete;

public:
	CConfiguration() = default;

	const CWordAttributes& Attributes() const { return wordAttributes; }
	void SetAttributes( CWordAttributes&& attributes );

private:
	CWordAttributes wordAttributes;
};

typedef shared_ptr<CConfiguration> CConfigurationPtr;

///////////////////////////////////////////////////////////////////////////////

const char* JsonConfigurationSchemeText();

CConfigurationPtr LoadConfigurationFromFile( const char* filename,
	ostream& errorStream, ostream& logStream );

///////////////////////////////////////////////////////////////////////////////

} // end of Configuration namespace
} // end of Lspl namespace
