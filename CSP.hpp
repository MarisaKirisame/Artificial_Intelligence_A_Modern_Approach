#ifndef CSP_HPP
#define CSP_HPP
#include <set>
#include <vector>
#include <cassert>
#include <algorithm>
#include <map>
#include <list>
#include <boost/any.hpp>
template< typename VARIABLE_ID_T >
struct constraint
{
	std::vector< VARIABLE_ID_T > related_var;
	std::function< bool( std::vector< std::reference_wrapper< const boost::any > > ) > predicate;
	constraint( const std::vector< VARIABLE_ID_T > & v, const std::function< bool( std::vector< std::reference_wrapper< const boost::any > > ) > & f ) :
		related_var( v ), predicate( f ) { }
	bool operator ( )( const std::map< VARIABLE_ID_T, boost::any > & partial_assignment ) const
	{
		std::vector< std::reference_wrapper< const boost::any > > arg;
		arg.reserve( related_var.size( ) );
		for ( const VARIABLE_ID_T & i : related_var )
		{
			auto it = partial_assignment.find( i );
			if ( it == partial_assignment.end( ) ) { return true; }
			arg.push_back( it->second );
		}
		return predicate( arg );
	}
};

template< typename VARIABLE_ID_T >
constraint< VARIABLE_ID_T > make_constraint(
		const std::vector< VARIABLE_ID_T > & v,
		const std::function< bool( std::vector< std::reference_wrapper< const boost::any > > ) > & f )
{ return constraint< VARIABLE_ID_T >( v, f ); }

template< typename VARIABLE_ID_T, typename ITER, typename OUTITER >
OUTITER backtracking_search(
		const std::map< VARIABLE_ID_T, std::list< boost::any > > & variable,
		const std::map< VARIABLE_ID_T, boost::any > & partial_assignment,
		ITER constraint_begin, ITER constraint_end, OUTITER result )
{
	assert( partial_assignment.size( ) <= variable.size( ) );
	if (
			std::any_of(
				constraint_begin,
				constraint_end,
				[&]( const constraint< VARIABLE_ID_T > & con ) { return ! con( partial_assignment ); } ) ) { return result; }
	if ( partial_assignment.size( ) == variable.size( ) )
	{
		* result = std::move( partial_assignment );
		++result;
		return result;
	}
	const std::pair< VARIABLE_ID_T, std::list< boost::any > > & next_element = * std::min_element(
				variable.begin( ),
				variable.end( ),
				[&]( const std::pair< VARIABLE_ID_T, std::list< boost::any > > & l, const std::pair< VARIABLE_ID_T, std::list< boost::any > > & r )
				{
					return ( partial_assignment.count( l.first ) == 0 ? l.second.size( ) : std::numeric_limits< size_t >::max( ) ) <
							( partial_assignment.count( r.first ) == 0 ? r.second.size( ) : std::numeric_limits< size_t >::max( ) );
				} );
	assert( partial_assignment.count( next_element.first ) == 0 );
	std::map< VARIABLE_ID_T, boost::any > ass( partial_assignment );
	for ( const boost::any & t : next_element.second )
	{
		ass.insert( std::make_pair( next_element.first, t ) );
		result = backtracking_search( variable, ass, constraint_begin, constraint_end, result );
		ass.erase( next_element.first );
	}
	return result;
}

template< typename VARIABLE_ID_T, typename ITER, typename OUTITER >
OUTITER backtracking_search( const std::map< VARIABLE_ID_T, std::list< boost::any > > & variable, ITER constrain_begin, ITER constrain_end, OUTITER result )
{ return backtracking_search( variable, std::map< VARIABLE_ID_T, boost::any >( ), constrain_begin, constrain_end, result ); }

#endif // CSP_HPP
