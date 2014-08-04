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
#include <functional>
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/function_output_iterator.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/test/unit_test.hpp>
template< typename STATE, typename EXPAND, typename RETURN_IF, typename OUTITER >
OUTITER breadth_first_search( const STATE & inital_state, EXPAND f1, RETURN_IF f2, OUTITER result )
{
	std::list< std::pair< STATE, std::list< STATE > > > in_search( { { inital_state, std::list< STATE >( { inital_state } ) } } );
	while ( ! in_search.empty( ) )
	{
		const auto & current_state = in_search.front( );
		if ( f2( current_state.first ) )
		{ return std::copy( current_state.second.begin( ), current_state.second.end( ), result ); }
		f1( current_state.first,
			boost::make_function_output_iterator( [&]( const STATE & s )
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
	return result;
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
	std::list< location > res;
	breadth_first_search( Sibiu,
						  [&](location l, const auto & it)
	{
		auto tem = map( ).equal_range( l );
		std::copy( boost::make_transform_iterator( tem.first, sf ),
				   boost::make_transform_iterator( tem.second, sf ),
				   it );
	},
						  [](location l){ return l == Bucharest; },
						  std::back_inserter( res ) );
	BOOST_CHECK_EQUAL( res, std::list< location >( { Sibiu, Fagaras, Bucharest } ) );
}

template< typename STATE, typename COST, typename EXPAND, typename RETURN_IF, typename COST_OUTPUT, typename OUTITER >
OUTITER uniform_cost_search( const STATE & inital_state,
							 const COST & inital_cost,
							 EXPAND f1,
							 RETURN_IF f2,
							 COST_OUTPUT f3,
							 OUTITER result )
{ return best_first_search( inital_state, inital_cost, f1, f2, [](const STATE &, const COST  & s){return s;}, f3, result ); }

template< typename STATE, typename COST, typename EXPAND, typename RETURN_IF, typename COST_OUTPUT, typename EVAL_FUNC, typename OUTITER >
OUTITER best_first_search( const STATE & inital_state,
						   const COST & inital_cost,
						   EXPAND f1,
						   RETURN_IF f2,
						   EVAL_FUNC f3,
						   COST_OUTPUT f4,
						   OUTITER result )
{
	struct state_tag { };
	struct eval_tag { };
	using namespace boost;
	using namespace multi_index;
	typedef decltype( f3( std::declval< STATE >( ), std::declval< COST >( ) ) ) EVAL;
	struct element
	{
		STATE state;
		COST cost;
		std::list< STATE > history;
		EVAL eval;
		element( const STATE & state, const COST & cost, const std::list< STATE > history, const EVAL & eval ) :
			state( state ), cost( cost ), history( history ), eval( eval ) { }
	};
	auto make_element = [&]( const STATE & state, const COST & cost, const std::list< STATE > &  history )
	{ return element( state, cost, history, f3( state, cost ) ); };
	auto update_element = [&]( element & e ){ e.eval = f3( e.state, e.cost ); };
	multi_index_container
	<
		element,
		indexed_by
		<
			ordered_unique< tag< state_tag >, member< element, STATE, & element::state > >,
			ordered_non_unique< tag< eval_tag >, member< element, EVAL, & element::eval > >
		>
	> container( { make_element( inital_state, inital_cost, { inital_state } ) } );
	auto & goodness_index = container.get< eval_tag >( );
	auto & state_index = container.get< state_tag >( );
	while ( ! container.empty( ) )
	{
		auto iterator = goodness_index.begin( );
		const element & current_element = * iterator;
		if ( f2( current_element.state ) )
		{
			f4( current_element.cost );
			return std::copy( current_element.history.begin( ), current_element.history.end( ), result );
		}
		f1( current_element.state, boost::make_function_output_iterator( [&]( const std::pair< STATE, COST > & e )
		{
			if ( std::count( current_element.history.begin( ), current_element.history.end( ), e.first ) != 0 ) { return; }
			auto it = state_index.find( e.first );
			COST cost = e.second + current_element.cost;
			std::list< STATE > history = current_element.history;
			history.push_back( e.first );
			if ( it == state_index.end( ) ) { state_index.insert( make_element( e.first, cost, std::move( history ) ) ); }
			else { state_index.modify( it, [&]( element & ee )
			{
				if ( ee.cost > cost )
				{
					ee.cost = cost;
					ee.history = std::move( history );
					update_element( ee );
				}
			} ); }
		} ) );
		goodness_index.erase( iterator );
	}
	return result;
}

BOOST_AUTO_TEST_CASE( UCS_TEST )
{
	auto sf = []( const std::pair< location, std::pair< location, size_t > > & pp ){ return pp.second; };
	std::list< location > res;
	uniform_cost_search( Sibiu,
						 0,
						 [&]( location l, auto it )
	{
		auto tem = map( ).equal_range( l );
		std::copy( boost::make_transform_iterator( tem.first, sf ), boost::make_transform_iterator( tem.second, sf ), it );
	},
						 [](location l){ return l == Bucharest; },
						 []( auto cost ){ BOOST_CHECK_EQUAL( cost, 278 ); },
						 std::back_inserter( res ) );
	BOOST_CHECK_EQUAL( res, std::list< location >( { Sibiu, Rimnicu_Vilcea, Pitesti, Bucharest } ) );
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

template< typename STATE, typename EXPAND, typename RETURN_IF, typename OUTITER >
OUTITER depth_first_search( const STATE & inital_state,
							EXPAND f1,
							RETURN_IF f2,
							OUTITER result )
{
	return depth_first_search( inital_state, f1, f2, postive_infinity( ), result );
}

template< typename STATE, typename EXPAND, typename RETURN_IF, typename NUM, typename OUTITER >
OUTITER depth_first_search( const STATE & inital_state,
							EXPAND f1,
							RETURN_IF f2,
							const NUM & depth,
							OUTITER result )
{
	if ( f2( inital_state ) )
	{
		* result = inital_state;
		++result;
		return result;
	}
	if ( depth < 1 ) { return result; }
	auto new_depth = depth - 1;
	std::vector< STATE > vec;
	f1( inital_state, std::back_inserter( vec ) );
	for ( const STATE & s : vec )
	{
		bool founded = false;
		depth_first_search( s, f1, f2, new_depth,
							boost::make_function_output_iterator( std::function< void( const STATE & ) >( [&]( const auto & state )
		{
			if ( ! founded )
			{
				* result = inital_state;
				++result;
				founded = true;
			}
			* result = state;
			++result;
		} ) ) );
		if ( founded ) { return result; }
	}
	return result;
}

template< typename STATE, typename EXPAND, typename RETURN_IF, typename OUTITER >
OUTITER iterative_deepening_depth_first_search( const STATE & inital_state,
												EXPAND f1,
												RETURN_IF f2,
												OUTITER result )
{
	size_t i = 0;
	bool break_loop = false;
	while ( ! break_loop )
	{
		depth_first_search( inital_state, f1, f2, i, boost::make_function_output_iterator( [&]( const STATE & s )
		{
			break_loop = true;
			* result = s;
			++result;
		} ) );
		++i;
	}
	return result;
}

BOOST_AUTO_TEST_CASE( DFS )
{
	auto sf = []( const std::pair< location, std::pair< location, size_t > > & pp ){ return pp.second.first; };
	std::list< location > res;
	depth_first_search( Sibiu,
						[&](location l, const auto & it)
	{
		auto tem = map( ).equal_range( l );
		std::copy( boost::make_transform_iterator( tem.first, sf ),
				   boost::make_transform_iterator( tem.second, sf ),
				   it );
	},
						[](location l){ return l == Bucharest; },
						2,
						std::back_inserter( res ) );
	BOOST_CHECK_EQUAL( res, std::list< location >( { Sibiu, Fagaras, Bucharest } ) );
}

BOOST_AUTO_TEST_CASE( IDDFS )
{
	auto sf = []( const std::pair< location, std::pair< location, size_t > > & pp ){ return pp.second.first; };
	std::list< location > res;
	iterative_deepening_depth_first_search( Sibiu,
											[&](location l, const auto & it)
	{
		auto tem = map( ).equal_range( l );
		std::copy( boost::make_transform_iterator( tem.first, sf ),
				   boost::make_transform_iterator( tem.second, sf ),
				   it );
	},
											[](location l){ return l == Bucharest; },
											std::back_inserter( res ) );
	BOOST_CHECK_EQUAL( res, std::list< location >( { Sibiu, Fagaras, Bucharest } ) );
}

template< typename STATE, typename FORWARD, typename BACKWARD, typename OUTITER >
OUTITER biderectional_breadth_first_search( const STATE & inital_state,
															const STATE & final_state,
															FORWARD f1,
															BACKWARD f2,
															OUTITER result )
{
	std::list< std::pair< STATE, std::list< STATE > > >
			forward( { { inital_state, std::list< STATE >( { inital_state } ) } } ),
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
			return std::copy( current_state.second.begin( ), current_state.second.end( ), result );
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
	return result;
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
	std::list< location > res;
	biderectional_breadth_first_search( Lugoj, Fagaras, expand, expand, std::back_inserter( res ) );
	BOOST_CHECK_EQUAL( res, std::list< location >( { Lugoj, Timisoara, Arad, Sibiu, Fagaras } ) );
}

#endif // SEARCH_HPP
