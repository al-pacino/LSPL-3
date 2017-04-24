#pragma once

namespace Lspl {

///////////////////////////////////////////////////////////////////////////////

template<typename VALUE_TYPE, typename SIZE_TYPE>
class CFixedSizeArray {
public:
	typedef VALUE_TYPE ValueType;
	typedef SIZE_TYPE SizeType;

	explicit CFixedSizeArray( const SizeType _size = 0 ) :
		size( 0 ),
		values( nullptr )
	{
		if( _size > 0 ) {
			size = _size;
			values = new ValueType[size];
			check_logic( values != nullptr );
		}
	}

	CFixedSizeArray( const CFixedSizeArray& another )
	{
		*this = another;
	}

	CFixedSizeArray& operator=( const CFixedSizeArray& another )
	{
		ValueType* tmp = new ValueType[another.size];
		check_logic( tmp != nullptr );
		for( SizeType i = 0; i < another.size; i++ ) {
			new( tmp + i )ValueType( another.values[i] );
		}
		values = tmp;
		size = another.size;
		return *this;
	}

	CFixedSizeArray( CFixedSizeArray&& another )
	{
		*this = move( another );
	}

	CFixedSizeArray& operator=( CFixedSizeArray&& another )
	{
		values = another.values;
		another.values = nullptr;
		size = another.size;
		another.size = 0;
		return *this;
	}

	const SizeType Size() const { return size; }
	const ValueType& operator[]( const SizeType index ) const
	{
		debug_check_logic( index < size );
		return values[index];
	}
	ValueType& operator[]( const SizeType index )
	{
		debug_check_logic( index < size );
		return values[index];
	}

private:
	SizeType size;
	ValueType* values;
};

#ifdef _DEBUG
void TestFixedSizeArray()
{
	typedef CFixedSizeArray<int, int> CSimple;

	CSimple empty;
	CSimple fsa7( 7 );
	CSimple fsa95( 95 );

	for( int i = 0; i < 95; i++ ) {
		fsa95[i] = i;
	}
	empty = fsa95;
	for( int i = 0; i < 95; i++ ) {
		debug_check_logic( fsa95[i] == i );
		debug_check_logic( empty[i] == i );
	}

	for( int i = 0; i < 7; i++ ) {
		fsa7[i] = 2 * i;
	}
	empty = move( fsa7 );
	debug_check_logic( fsa7.Size() == 0 );
	for( int i = 0; i < 7; i++ ) {
		debug_check_logic( empty[i] == 2 * i );
	}
}
#endif

///////////////////////////////////////////////////////////////////////////////

} // end of Lspl namespace
