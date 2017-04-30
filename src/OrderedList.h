#pragma once

namespace Lspl {

///////////////////////////////////////////////////////////////////////////////

template<typename VALUE_TYPE>
class COrderedList {
public:
	typedef VALUE_TYPE ValueType;
	typedef typename vector<ValueType>::size_type SizeType;

	COrderedList() = default;

	void Empty() { values.clear(); }
	bool IsEmpty() const { return values.empty(); }
	SizeType Size() const { return values.size(); }
	const ValueType& Value( const SizeType index ) const;
	bool Add( const ValueType& value );
	bool Has( const ValueType& value ) const;
	bool Erase( const ValueType& value );
	bool Find( const ValueType& value, SizeType& index ) const;
	template<typename CAST_TYPE = VALUE_TYPE>
	void Print( ostream& out, const char* const delimiter ) const;

	static COrderedList Union( const COrderedList&, const COrderedList& );
	static COrderedList Difference( const COrderedList&, const COrderedList& );
	static COrderedList Intersection( const COrderedList&, const COrderedList& );

private:
	vector<ValueType> values;
};

template<typename VALUE_TYPE>
const VALUE_TYPE& COrderedList<VALUE_TYPE>::Value( const SizeType index ) const
{
	debug_check_logic( index < Size() );
	return values[index];
}

template<typename VALUE_TYPE>
bool COrderedList<VALUE_TYPE>::Add( const ValueType& value )
{
	auto i = lower_bound( values.begin(), values.end(), value );
	if( i != values.cend() && *i == value ) {
		return false;
	}
	values.insert( i, value );
	return true;
}

template<typename VALUE_TYPE>
bool COrderedList<VALUE_TYPE>::Has( const ValueType& value ) const
{
	return binary_search( values.cbegin(), values.cend(), value );
}

template<typename VALUE_TYPE>
bool COrderedList<VALUE_TYPE>::Erase( const ValueType& value )
{
	auto i = lower_bound( values.cbegin(), values.cend(), value );
	if( i != values.cend() && *i == value ) {
		values.erase( i );
		return true;
	}
	return false;
}

template<typename VALUE_TYPE>
bool COrderedList<VALUE_TYPE>::Find( const ValueType& value,
	SizeType& index ) const
{
	auto i = lower_bound( values.cbegin(), values.cend(), value );
	if( i != values.cend() && *i == value ) {
		index = i - values.cbegin();
		return true;
	}
	return false;
}

template<typename VALUE_TYPE>
template<typename CAST_TYPE>
void COrderedList<VALUE_TYPE>::Print( ostream& out,
	const char* const delimiter ) const
{
	auto i = values.cbegin();
	if( i != values.cend() ) {
		out << static_cast<CAST_TYPE>( *i );
		for( ++i; i != values.cend(); ++i ) {
			out << delimiter << static_cast<CAST_TYPE>( *i );
		}
	}
}

template<typename VALUE_TYPE>
COrderedList<VALUE_TYPE> COrderedList<VALUE_TYPE>::Union(
	const COrderedList& a, const COrderedList& b )
{
	COrderedList result;
	set_union( a.values.cbegin(), a.values.cend(),
		b.values.cbegin(), b.values.cend(),
		back_inserter( result.values ) );
	return result;
}

template<typename VALUE_TYPE>
COrderedList<VALUE_TYPE> COrderedList<VALUE_TYPE>::Difference(
	const COrderedList& a, const COrderedList& b )
{
	COrderedList result;
	set_difference( a.values.cbegin(), a.values.cend(),
		b.values.cbegin(), b.values.cend(),
		back_inserter( result.values ) );
	return result;
}

template<typename VALUE_TYPE>
COrderedList<VALUE_TYPE> COrderedList<VALUE_TYPE>::Intersection(
	const COrderedList& a, const COrderedList& b )
{
	COrderedList result;
	set_intersection( a.values.cbegin(), a.values.cend(),
		b.values.cbegin(), b.values.cend(),
		back_inserter( result.values ) );
	return result;
}

///////////////////////////////////////////////////////////////////////////////

} // end of Lspl namespace
