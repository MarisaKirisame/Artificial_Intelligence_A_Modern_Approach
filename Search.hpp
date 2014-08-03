#ifndef SEARCH_HPP
#define SEARCH_HPP
#include <boost/optional/optional.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <list>
#include <set>
#include <iostream>
#include <map>
#include <utility>
#include <cassert>
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/function_output_iterator.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/test/unit_test.hpp>
template< typename STATE, typename EXPAND, typename RETURN_IF >
boost::optional< std::list< STATE > > breadth_first_search( const STATE & inital_state, const EXPAND & f1, const RETURN_IF & f2 )
{
	std::list< std::pair< STATE, std::list< STATE > > > in_search( { { inital_state, std::list< STATE >( ) } } );
	while ( ! in_search.empty( ) )
	{
		const auto & current_state = in_search.front( );
		if ( f2( current_state.first ) ) { return current_state.second; }
		f1( current_state.first,
			boost::make_function_output_iterator( [&](const STATE & s)
		{
			in_search.push_back( { s,
								   [&]( )
								   {
									   auto ret = current_state.second;
									   ret.push_back(s);
									   return ret;
								   }( )
								 } );
		} ) );
		in_search.pop_front( );
	}
	return boost::optional< std::list< STATE > >( );
}
enum location
{
	Sibiu, Fagaras, Bucharest, Pitesti, Rimnicu_Vilcea,
	Oradea, Zerind, Arad, Timisoara, Lugoj,
	Mehadia, Drobeta, Craivoa, Giurgiu, Urziceni,
	Hirsova, Eforie, Vaslui, Iasi, Neamt
};

const std::multimap< location, std::pair< location, size_t > > map( )
{
	static std::multimap< location, std::pair< location, size_t > > ret( []()
	{
		std::multimap< location, std::pair< location, size_t > > ret;
		auto add_edge = [&]( location a, location b, size_t cost )
		{
			ret.insert( { a, { b, cost } } );
			ret.insert( { b, { a, cost } } );
		};
		add_edge( Sibiu, Rimnicu_Vilcea, 80 );
		add_edge( Rimnicu_Vilcea, Pitesti, 97 );
		add_edge( Pitesti, Bucharest, 101 );
		add_edge( Bucharest, Fagaras, 211 );
		add_edge( Fagaras, Sibiu, 99 );
		add_edge( Oradea, Zerind, 71 );
		add_edge( Zerind, Arad, 75 );
		add_edge( Arad, Timisoara, 118 );
		add_edge( Oradea, Sibiu, 151 );
		add_edge( Arad, Sibiu, 140 );
		add_edge( Timisoara, Lugoj, 111 );
		add_edge( Lugoj, Mehadia, 70 );
		add_edge( Mehadia, Drobeta, 75 );
		add_edge( Drobeta, Craivoa, 120 );
		add_edge( Craivoa, Rimnicu_Vilcea, 146 );
		add_edge( Craivoa, Pitesti, 138 );
		add_edge( Bucharest, Giurgiu, 90 );
		add_edge( Bucharest, Urziceni, 85 );
		add_edge( Urziceni, Hirsova, 98 );
		add_edge( Hirsova, Eforie, 86 );
		add_edge( Urziceni, Vaslui, 142 );
		add_edge( Vaslui, Iasi, 92 );
		add_edge( Iasi, Neamt, 87 );
		return ret;
	}( ) );
	return ret;
}
BOOST_TEST_DONT_PRINT_LOG_VALUE( std::list< location > );
BOOST_AUTO_TEST_CASE( BFS_TEST )
{
	auto sf = []( const std::pair< location, std::pair< location, size_t > > & pp ){ return pp.second.first; };
	auto res = breadth_first_search( Sibiu,
									 [&](location l, const auto & it)
	{
		auto tem = map( ).equal_range( l );
		std::copy( boost::make_transform_iterator( tem.first, sf ),
				   boost::make_transform_iterator( tem.second, sf ),
				   it );
	},
		[](location l){ return l == Bucharest; } );
	BOOST_CHECK_EQUAL( res, std::list< location >( { Fagaras, Bucharest } ) );
}

template< typename STATE, typename COST, typename EXPAND, typename RETURN_IF >
boost::optional< std::pair< std::list< STATE >, COST > > uniform_cost_search( const STATE & inital_state,
																			  const COST & inital_cost,
																			  const EXPAND & f1,
																			  const RETURN_IF & f2 )
{
	struct state_tag { };
	struct cost_tag { };
	using namespace boost;
	using namespace multi_index;
	struct element
	{
		STATE state;
		COST cost;
		std::list< STATE > history;
	};
	multi_index_container
	<
		element,
		indexed_by
		<
			ordered_unique< tag< state_tag >, member< element, STATE, & element::state > >,
			ordered_non_unique< tag< cost_tag >, member< element, COST, & element::cost > >
		>
	> container( { { inital_state, inital_cost, { } } } );
	auto & cost_index = container.get< cost_tag >( );
	auto & state_index = container.get< state_tag >( );
	while ( ! container.empty( ) )
	{
		auto iterator = cost_index.begin( );
		const element & current_element = * iterator;
		if ( f2( current_element.state ) ) { return std::make_pair( current_element.history, current_element.cost ); }
		f1( current_element.state, boost::make_function_output_iterator( [&]( const std::pair< STATE, COST > & e )
		{
			auto it = state_index.find( e.first );
			COST cost = e.second + current_element.cost;
			std::list< STATE > history = current_element.history;
			history.push_back( e.first );
			if ( it == state_index.end( ) ) { state_index.insert( element( { e.first, cost, std::move( history ) } ) ); }
			else { state_index.modify( it, [&]( element & ee )
			{
				if ( ee.cost > cost )
				{
					ee.cost = cost;
					ee.history = std::move( history );
				}
			} ); }
		} ) );
		cost_index.erase( iterator );
	}
	return boost::optional< std::pair< std::list< STATE >, COST > >( );
}
typedef std::pair<std::list<location>,size_t> ignore;
BOOST_TEST_DONT_PRINT_LOG_VALUE( ignore );
BOOST_AUTO_TEST_CASE( UCS_TEST )
{
	auto sf = []( const std::pair< location, std::pair< location, size_t > > & pp ){ return pp.second; };
	auto res = uniform_cost_search( Sibiu,
									static_cast< size_t >( 0 ),
									[&]( location l, auto it )
	{
		auto tem = map( ).equal_range( l );
		std::copy( boost::make_transform_iterator( tem.first, sf ), boost::make_transform_iterator( tem.second, sf ), it );
	},
									[](location l){ return l == Bucharest; } );
	BOOST_CHECK_EQUAL( res, std::make_pair( std::list< location >( { Rimnicu_Vilcea, Pitesti, Bucharest } ), static_cast< size_t >( 278 ) ) );
}

struct postive_infinity
{
	bool operator == ( const postive_infinity & ) { return true; }
	template< typename T >
	bool operator == ( const T & ) const { return false; }
	template< typename T >
	bool operator != ( const T & t ) const { return ! ( ( * this ) == t ); }
	template< typename T >
	bool operator < ( const T & ) const { return false; }
	template< typename T >
	bool operator >= ( const T & ) const { return true; }
	template< typename T >
	bool operator <= ( const T & t ) const { return ( * this ) == t; }
	template< typename T >
	bool operator > ( const T & t ) const { return ( * this ) != t; }
	template< typename T >
	postive_infinity operator + ( const T & ) const { return * this; }
	template< typename T >
	postive_infinity operator - ( const T & ) const { return * this; }
	postive_infinity operator - ( const postive_infinity & ) const = delete;
	template< typename T >
	postive_infinity operator * ( const T & t ) const
	{
		assert( t > 0 );
		return * this;
	}
	template< typename T >
	postive_infinity operator / ( const T & t ) const
	{
		assert( t > 0 );
		return * this;
	}
	template< typename T >
	postive_infinity & operator += ( const T & t )
	{
		( * this ) = ( * this ) + t;
		return * this;
	}
	template< typename T >
	postive_infinity & operator -= ( const T & t )
	{
		( * this ) = ( * this ) - t;
		return * this;
	}
	template< typename T >
	postive_infinity & operator *= ( const T & t )
	{
		( * this ) = ( * this ) * t;
		return * this;
	}
	template< typename T >
	postive_infinity & operator /= ( const T & t )
	{
		( * this ) = ( * this ) / t;
		return * this;
	}
};

template< typename STATE, typename EXPAND, typename RETURN_IF >
boost::optional< std::list< STATE > > depth_first_search( const STATE & inital_state,
														  const EXPAND & f1,
														  const RETURN_IF & f2 )
{
	return depth_first_search( inital_state, f1, f2, postive_infinity( ) );
}

template< typename STATE, typename EXPAND, typename RETURN_IF, typename NUM >
boost::optional< std::list< STATE > > depth_first_search( const STATE & inital_state,
														  const EXPAND & f1,
														  const RETURN_IF & f2,
														  const NUM & depth,
														  bool outside_call = true )
{
	if ( f2( inital_state ) ) { return outside_call ? std::list< STATE >( ) : std::list< STATE >( { inital_state }  ); }
	typedef boost::optional< std::list< STATE > > ret_type;
	if ( depth < 1 ) { return ret_type( ); }
	auto new_depth = depth - 1;
	std::vector< STATE > vec;
	f1( inital_state, std::back_inserter( vec ) );
	for ( const STATE & s : vec )
	{
		auto res = depth_first_search( s, f1, f2, new_depth, false );
		if ( res )
		{
			if ( ! outside_call ) { res->push_front( inital_state ); }
			return res;
		}
	}
	return ret_type( );
}

template< typename STATE, typename EXPAND, typename RETURN_IF >
boost::optional< std::list< STATE > > iterative_deepening_depth_first_search( const STATE & inital_state,
														  const EXPAND & f1,
														  const RETURN_IF & f2 )
{
	size_t i = 0;
	while ( true )
	{
		auto res = depth_first_search( inital_state, f1, f2, i );
		++i;
		if ( res ) { return res; }
	}
}

BOOST_AUTO_TEST_CASE( DFS )
{
	auto sf = []( const std::pair< location, std::pair< location, size_t > > & pp ){ return pp.second.first; };
	auto res = depth_first_search( Sibiu,
									 [&](location l, const auto & it)
	{
		auto tem = map( ).equal_range( l );
		std::copy( boost::make_transform_iterator( tem.first, sf ),
				   boost::make_transform_iterator( tem.second, sf ),
				   it );
	},
				 [](location l){ return l == Bucharest; },
				 2 );
	BOOST_CHECK_EQUAL( res, std::list< location >( { Fagaras, Bucharest } ) );
}

BOOST_AUTO_TEST_CASE( IDDFS )
{
	auto sf = []( const std::pair< location, std::pair< location, size_t > > & pp ){ return pp.second.first; };
	auto res = iterative_deepening_depth_first_search( Sibiu,
									 [&](location l, const auto & it)
	{
		auto tem = map( ).equal_range( l );
		std::copy( boost::make_transform_iterator( tem.first, sf ),
				   boost::make_transform_iterator( tem.second, sf ),
				   it );
	},
				 [](location l){ return l == Bucharest; } );
	BOOST_CHECK_EQUAL( res, std::list< location >( { Fagaras, Bucharest } ) );
}

template< typename STATE, typename FORWARD, typename BACKWARD >
boost::optional< std::list< STATE > > biderectional_breadth_first_search( const STATE & inital_state,
															const STATE & final_state,
															const FORWARD & f1,
															const BACKWARD & f2 )
{
	std::list< std::pair< STATE, std::list< STATE > > >
			forward( { { inital_state, std::list< STATE >( ) } } ),
			backward( { { final_state, std::list< STATE >( { final_state } ) } } );
	std::map< STATE, std::list< STATE > >
			forward_expanded( { { inital_state, std::list< STATE >( ) } } ),
			backward_expanded( { { final_state, std::list< STATE >( ) } } );
	bool do_forward = true;
	auto expand = [&]( const STATE & s, auto it )
	{
		if ( do_forward ) { f1( s, it ); }
		else { f2( s, it ); }
	};
	while ( ( ! forward.empty( ) ) || ( ! backward.empty( ) ) )
	{
		auto & current_map = do_forward ? forward : backward;
		auto & detect_map = do_forward ? backward_expanded : forward_expanded;
		auto & expand_map = do_forward ? forward_expanded : backward_expanded;
		auto & current_state = current_map.front( );
		if ( detect_map.count( current_state.first ) != 0 )
		{
			current_state.second.pop_back( );
			( do_forward ? detect_map.find( current_state.first )->second : current_state.second ).reverse( );
			current_state.second.splice( current_state.second.end( ), detect_map.find( current_state.first )->second );
			return current_state.second;
		}
		expand( current_state.first,
				boost::make_function_output_iterator( [&](const STATE & s)
		{
			current_map.push_back( { s,
									   [&]( )
									   {
										   auto ret = current_state.second;
										   ret.push_back(s);
										   return ret;
									   }( )
									} );
		} ) );
		expand_map.insert( current_state );
		current_map.pop_front( );
		do_forward = ! do_forward;
	}
	return boost::optional< std::list< STATE > >( );
}

BOOST_AUTO_TEST_CASE( BBFS )
{
	auto sf = []( const std::pair< location, std::pair< location, size_t > > & pp ){ return pp.second.first; };
	auto expand = [&]( location l, const auto & it )
	{
		auto tem = map( ).equal_range( l );
		std::copy( boost::make_transform_iterator( tem.first, sf ),
				   boost::make_transform_iterator( tem.second, sf ),
				   it );
	};
	auto res = biderectional_breadth_first_search( Lugoj, Fagaras, expand, expand );
	BOOST_CHECK_EQUAL( res, std::list< location >( { Timisoara, Arad, Sibiu, Fagaras } ) );
}

#endif // SEARCH_HPP
