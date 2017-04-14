#pragma once

#include <Tokenizer.h>
#include <Pattern.h>

namespace Lspl {

///////////////////////////////////////////////////////////////////////////////

class CTranspositionSupport {
	CTranspositionSupport( const CTranspositionSupport& ) = delete;
	CTranspositionSupport& operator=( const CTranspositionSupport& ) = delete;

public:
	static const size_t MaxTranspositionSize = 9;

	struct CSwap : public pair<size_t, size_t> {
		template<typename ValueType>
		void Apply( vector<ValueType>& vect ) const
		{
			debug_check_logic( first < second );
			debug_check_logic( second < vect.size() );
			ValueType tmp = move( vect[first] );
			vect[first] = move( vect[second] );
			vect[second] = move( tmp );
		}
	};
	typedef vector<CSwap> CSwaps;

	static const CTranspositionSupport& Instance();
	const CSwaps& Swaps( const size_t transpositionSize ) const;

private:
	static unique_ptr<CTranspositionSupport> instance; // singleton
	mutable unordered_map<size_t, CSwaps> allSwaps;

	CTranspositionSupport() = default;
	static void fillSwaps( CSwaps& swaps, const size_t size );

	typedef basic_string<unsigned char> CTransposition;
	typedef list<CTransposition> CTranspositions;
	static CTranspositions generate( const CTransposition& first );
	static bool connect( const CTransposition& first,
		const CTransposition& second, CSwaps& swaps );
};

///////////////////////////////////////////////////////////////////////////////

} // end of Lspl namespace
