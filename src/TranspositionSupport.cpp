#include <common.h>
#include <TranspositionSupport.h>

namespace Lspl {

///////////////////////////////////////////////////////////////////////////////

unique_ptr<CTranspositionSupport> CTranspositionSupport::instance;

const CTranspositionSupport& CTranspositionSupport::Instance()
{
	if( !static_cast<bool>( instance ) ) {
		instance.reset( new CTranspositionSupport );
	}
	return *instance;
}

const CTranspositionSupport::CSwaps&
CTranspositionSupport::Swaps( const size_t size ) const
{
	check_logic( size <= MaxTranspositionSize );

	auto p = allSwaps.insert( make_pair( size, CSwaps() ) );
	if( p.second ) {
		fillSwaps( p.first->second, size );
	}
	return p.first->second;
}

void CTranspositionSupport::fillSwaps( CSwaps& swaps, const size_t size )
{
	debug_check_logic( swaps.empty() );

	CTransposition first( size, 0 );
	for( CTransposition::value_type i = 0; i < size; i++ ) {
		first[i] = i;
	}

	CTranspositions transpositions = generate( first );
	if( transpositions.empty() ) {
		return;
	}

	CTransposition current = transpositions.front();
	transpositions.pop_front();
	while( !transpositions.empty() ) {
		for( auto i = transpositions.begin(); i != transpositions.end(); ++i ) {
			if( connect( *i, current, swaps ) ) {
				current = *i;
				transpositions.erase( i );
				break;
			}
		}
	}
}

CTranspositionSupport::CTranspositions
CTranspositionSupport::generate( const CTransposition& first )
{
	CTranspositions transpositions;
	if( first.length() == 1 ) {
		transpositions.push_back( first );
	} else if( !first.empty() ) {
		CTranspositions rest = generate( first.substr( 1 ) );
		for( const CTransposition& sub : rest ) {
			transpositions.push_back( first.substr( 0, 1 ) + sub );
		}
		for( const CTransposition& sub : rest ) {
			transpositions.push_back( sub + first.substr( 0, 1 ) );
		}
	}
	return transpositions;
}

bool CTranspositionSupport::connect( const CTransposition& first,
	const CTransposition& second, CSwaps& swaps )
{
	debug_check_logic( first.length() == second.length() );

	size_t difference = 0;
	CSwap swap;
	for( size_t i = 0; i < first.length(); i++ ) {
		if( first[i] != second[i] ) {
			difference++;
			if( difference > 2 ) {
				return false;
			} else if( difference == 2 ) {
				swap.second = i;
			} else if( difference == 1 ) {
				swap.first = i;
			}
		}
	}

	debug_check_logic( difference == 2 );
	debug_check_logic( first[swap.first] == second[swap.second] );
	debug_check_logic( first[swap.second] == second[swap.first] );

	swaps.push_back( swap );
	return true;
}

///////////////////////////////////////////////////////////////////////////////

} // end of Lspl namespace
