#ifndef CSP_HPP
#define CSP_HPP
#include <set>
#include <vector>
#include <cassert>
#include <algorithm>
template< typename T, typename ITER, typename OUTITER >
OUTITER backtracking_search( const std::vector< std::set< T > > & variable, ITER constrain_begin, ITER constrain_end, OUTITER result )
{ return backtracking_search( variable, { }, constrain_begin, constrain_end, result ); }

template< typename T, typename ITER, typename OUTITER >
OUTITER backtracking_search(
		const std::vector< std::set< T > > & variable, const std::vector< T > & partial_assignment, ITER constrain_begin, ITER constrain_end, OUTITER result )
{
	assert( partial_assignment.size( ) <= variable.size( ) );
	if ( std::any_of( constrain_begin, constrain_end, [&]( const auto & constraint ){ return ! constraint( partial_assignment ); } ) ) { return result; }
	if ( partial_assignment.size( ) == variable.size( ) )
	{
		* result = std::move( partial_assignment );
		++result;
		return result;
	}
	std::vector< T > vec( partial_assignment );
	std::set< T > & next_element = variable[partial_assignment.size( )];
	for ( const T & t : next_element )
	{
		vec.push_back( t );
		result = backtracking_search(  variable, vec, constrain_begin, constrain_end, result );
		vec.pop_back( );
	}
	return result;
}
#endif // CSP_HPP
