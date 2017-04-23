#pragma once

#include <OrderedList.h>

namespace Lspl {
namespace Configuration {

///////////////////////////////////////////////////////////////////////////////

enum TWordSignType {
	WST_None,
	WST_Main,
	WST_Enum,
	WST_String
};

typedef COrderedList<string> COrderedStrings;

struct CWordSign {
	bool Consistent;
	TWordSignType Type;
	COrderedStrings Names;
	COrderedStrings Values;

	CWordSign() :
		Consistent( false ),
		Type( WST_None )
	{
	}

	void Print( ostream& out ) const;
};

class CWordSigns {
	friend class CWordSignsBuilder;
	CWordSigns( const CWordSigns& ) = delete;
	CWordSigns& operator=( const CWordSigns& ) = delete;

public:
	typedef vector<CWordSign>::size_type SizeType;

	CWordSigns();
	bool IsEmpty() const;
	SizeType Size() const;
	SizeType MainWordSignIndex() const;
	const CWordSign& MainWordSign() const;
	const CWordSign& operator[]( SizeType index ) const;
	bool Find( const string& name, SizeType& index ) const;
	void Print( ostream& out ) const;

private:
	vector<CWordSign> wordSigns;
	typedef unordered_map<string, SizeType> CNameIndices;
	CNameIndices nameIndices;
};

class CWordSignsBuilder {
public:
	explicit CWordSignsBuilder( const CWordSigns::SizeType count );
	void Add( CWordSign&& wordSign );
	bool Build( ostream& errorStream, CWordSigns& wordSigns );

private:
	vector<CWordSign> mainSigns;
	vector<CWordSign> consistentSigns;
	vector<CWordSign> notConsistentSigns;
};

class CConfiguration {
	CConfiguration( const CConfiguration& ) = delete;
	CConfiguration& operator=( const CConfiguration& ) = delete;

public:
	const CWordSigns& WordSigns() const
	{
		return wordSigns;
	}

protected:
	CWordSigns wordSigns;

	CConfiguration()
	{
	}
};

typedef shared_ptr<CConfiguration> CConfigurationPtr;

const char* JsonConfigurationSchemeText();

CConfigurationPtr LoadConfigurationFromFile( const char* filename,
	ostream& errorStream, ostream& logStream );

///////////////////////////////////////////////////////////////////////////////

} // end of Configuration namespace
} // end of Lspl namespace
