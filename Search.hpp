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
	size_t inital_depth = 0;
	return uniform_cost_search( inital_state,
								inital_depth,
								[&]( const STATE & s, auto it )
								{
									f1( s,
									boost::make_function_output_iterator(
											[&]( const STATE & s )
											{
												*it = std::make_pair(s,1);
												++it;
											} ) );
								},
								f2,
								[](size_t){},
								result );
}

enum location
{
	Sibiu, Fagaras, Bucharest, Pitesti, Rimnicu_Vilcea,
	Oradea, Zerind, Arad, Timisoara, Lugoj,
	Mehadia, Drobeta, Craivoa, Giurgiu, Urziceni,
	Hirsova, Eforie, Vaslui, Iasi, Neamt
};

const std::multimap< location, std::pair< location, size_t > > & map( )
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
	breadth_first_search(
						Sibiu,
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
OUTITER uniform_cost_search(
		const STATE & inital_state,
		const COST & inital_cost,
		EXPAND f1,
		RETURN_IF f2,
		COST_OUTPUT f3,
		OUTITER result )
{ return best_first_search( inital_state, inital_cost, f1, f2, [](const STATE &, const COST & s){return s;}, f3, result ); }

template< typename STATE, typename COST, typename EXPAND, typename RETURN_IF, typename COST_OUTPUT, typename EVAL_FUNC, typename OUTITER >
OUTITER best_first_search(
		const STATE & inital_state,
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


template
<
	typename STATE, typename COST, typename EVAL,
	typename EXPAND, typename RETURN_IF, typename COST_OUTPUT, typename OUTITER
>
OUTITER recursive_uniform_cost_search(
		const STATE & inital_state,
		const COST & inital_cost,
		const EVAL & eval_limit,
		EXPAND f1,
		RETURN_IF f2,
		COST_OUTPUT f3,
		OUTITER result
		)
{
	return recursive_best_first_search(
				inital_state,
				inital_cost,
				boost::optional< EVAL >(),
				eval_limit,
				{},
				f1,
				f2,
				[]( const STATE &, const COST & s ){ return s; },
				f3,
				[]( const EVAL & ){},
				result
				);
}

template
<
	typename STATE, typename COST, typename EVAL,
	typename EXPAND, typename RETURN_IF, typename COST_OUTPUT, typename SEARCH_EVAL_OUTPUT, typename EVAL_FUNC, typename OUTITER
>
OUTITER recursive_best_first_search(
		const STATE & inital_state,
		const COST & inital_cost,
		const boost::optional< EVAL > & init_eval,
		const EVAL & eval_limit,
		const std::set< STATE > & search_before,
		EXPAND f1,
		RETURN_IF f2,
		EVAL_FUNC f3,
		COST_OUTPUT f4,
		SEARCH_EVAL_OUTPUT f5,
		OUTITER result )
{
	EVAL inital_eval( f3( inital_state, inital_cost ) );
	if ( inital_eval > eval_limit )
	{
		f5( inital_eval );
		return result;
	}
	if ( f2( inital_state ) )
	{
		f4( inital_cost );
		* result = inital_state;
		++result;
		return result;
	}
	std::set< STATE > param = search_before;
	param.insert( inital_state );
	struct state_tag { };
	struct eval_tag { };
	using namespace boost;
	using namespace multi_index;
	struct element
	{
		STATE state;
		COST cost;
		EVAL eval;
		element( const STATE & state, const COST & cost, const EVAL & eval ) :
			state( state ), cost( cost ), eval( eval ) { }
	};
	auto make_element = [&]( const STATE & state, const COST & cost )
	{ return element( state, cost, init_eval ? std::max( f3( state, cost ), * init_eval ) : f3( state, cost ) ); };
	multi_index_container
	<
		element,
		indexed_by
		<
			ordered_unique< tag< state_tag >, member< element, STATE, & element::state > >,
			ordered_non_unique< tag< eval_tag >, member< element, EVAL, & element::eval > >
		>
	> container;
	auto & goodness_index = container.get< eval_tag >( );
	f1(
				inital_state,
				boost::make_function_output_iterator(
					[&]( const std::pair< STATE, COST > & p )
	{
		if ( search_before.count( p.first ) == 0 ) { container.insert( make_element( p.first, p.second + inital_cost ) ); }
	} ) );
	if ( container.empty( ) )
	{
		f5( eval_limit * 2 );
		return result;
	}
	else if ( container.size( ) == 1 )
	{
		bool find_solution = false;
		recursive_best_first_search(
					container.begin( )->state,
					inital_cost + container.begin( )->cost,
					boost::optional< EVAL >( container.begin( )->eval ),
					eval_limit,
					param,
					f1,
					f2,
					f3,
					f4,
					f5,
					boost::make_function_output_iterator(
						std::function< void ( const STATE & ) >(
							[&]( const STATE & s )
							{
								if ( ! find_solution )
								{
									* result = inital_state;
									++result;
									find_solution = true;
								}
								* result = s;
								++ result;
							} ) ) );
		return result;
	}
	while ( true )
	{
		auto iterator = goodness_index.begin( );
		const element & current_element = * iterator;
		++iterator;
		const element & second_element = * iterator;
		if ( current_element.eval > eval_limit )
		{
			f5( current_element.eval );
			return result;
		}
		bool find_solution = false;
		recursive_best_first_search(
					current_element.state,
					current_element.cost,
					boost::optional< EVAL >( current_element.eval ),
					std::min( eval_limit, second_element.eval ),
					param,
					f1,
					f2,
					f3,
					f4,
					std::function< void ( const EVAL & ) >(
						[&](const EVAL & e)
						{
							goodness_index.modify( goodness_index.begin( ), [&](element & el){ el.eval = e; } );
							f5( e );
						} ),
					boost::make_function_output_iterator(
				std::function< void ( const STATE & ) >(
					[&]( const STATE & s )
					{
						if ( ! find_solution )
						{
							* result = inital_state;
							++result;
							find_solution = true;
						}
						* result = s;
						++ result;
					} ) ) );
		if ( find_solution )
		{
			return result;
		}
	}
	return result;
}

BOOST_AUTO_TEST_CASE( UCS_TEST )
{
	auto sf = []( const std::pair< location, std::pair< location, size_t > > & pp ){ return pp.second; };
	std::list< location > res;
	recursive_uniform_cost_search(
				Sibiu,
				0,
				10000,
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
OUTITER depth_first_search(
		const STATE & inital_state,
		EXPAND f1,
		RETURN_IF f2,
		OUTITER result )
{ return depth_first_search( inital_state, postive_infinity( ), f1, f2, result ); }

template< typename STATE, typename NUM, typename EXPAND, typename RETURN_IF, typename OUTITER >
OUTITER depth_first_search(
		const STATE & inital_state,
		const NUM & depth,
		EXPAND f1,
		RETURN_IF f2,
		OUTITER result )
{
	std::set< location > history = { inital_state };
	return depth_first_search( inital_state, depth, history, f1, f2, result );
}

template< typename STATE, typename NUM, typename EXPAND, typename RETURN_IF, typename OUTITER >
OUTITER depth_first_search(
		const STATE & inital_state,
		const NUM & depth,
		std::set< STATE > & history,
		EXPAND f1,
		RETURN_IF f2,
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
		if ( history.count( s ) != 0 ) { continue; }
		history.insert( s );
		bool find_solution = false;
		depth_first_search(
					s,
					new_depth,
					history,
					f1,
					f2,
					boost::make_function_output_iterator(
						std::function< void( const STATE & ) >(
							[&]( const auto & state )
							{
								if ( ! find_solution )
								{
									* result = inital_state;
									++result;
									find_solution = true;
								}
								* result = state;
								++result;
							} ) ) );
		if ( find_solution ) { return result; }
		history.erase( s );
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
		depth_first_search( inital_state, i, f1, f2, boost::make_function_output_iterator(
								[&]( const STATE & s )
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
							std::copy(
										boost::make_transform_iterator( tem.first, sf ),
										boost::make_transform_iterator( tem.second, sf ),
										it );
						},
						[](location l){ return l == Bucharest; },
						std::back_inserter( res ) );
	BOOST_CHECK_EQUAL( res.front( ), Sibiu );
	BOOST_CHECK_EQUAL( res.back( ), Bucharest );
}

BOOST_AUTO_TEST_CASE( IDDFS )
{
	auto sf = []( const std::pair< location, std::pair< location, size_t > > & pp ){ return pp.second.first; };
	std::list< location > res;
	iterative_deepening_depth_first_search(
				Sibiu,
				[&](location l, const auto & it)
				{
					auto tem = map( ).equal_range( l );
					std::copy(
								boost::make_transform_iterator( tem.first, sf ),
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
	auto expand =
			[&]( const STATE & s, auto it )
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

template< typename STATE, typename COST, typename EXPAND, typename EVAL, typename RETURN_IF, typename COST_OUTPUT, typename OUTITER >
OUTITER greedy_best_first_search(
		const STATE & inital_state,
		const COST & inital_cost,
		EXPAND f1,
		EVAL f2,
		RETURN_IF f3,
		COST_OUTPUT f4,
		OUTITER result )
{ return best_first_search( inital_state, inital_cost, f1, f3, [&](const STATE & s, const COST &){return f2(s);}, f4, result ); }

template< typename STATE, typename COST, typename EXPAND, typename EVAL, typename RETURN_IF, typename COST_OUTPUT, typename OUTITER >
OUTITER A_star(
		const STATE & inital_state,
		const COST & inital_cost,
		EXPAND f1,
		EVAL f2,
		RETURN_IF f3,
		COST_OUTPUT f4,
		OUTITER result )
{ return best_first_search( inital_state, inital_cost, f1, f3, [&](const STATE & s, const COST & c){return f2(s) + c;}, f4, result ); }

template
<
	typename STATE, typename COST,
	typename EXPAND, typename RETURN_IF, typename COST_OUTPUT, typename EVAL_FUNC, typename OUTITER
>
OUTITER memory_bounded_best_first_search(
		const STATE & inital_state,
		const COST & inital_cost,
		size_t container_limit,
		EXPAND f1,
		RETURN_IF f2,
		EVAL_FUNC f3,
		COST_OUTPUT f4,
		OUTITER result )
{
	typedef decltype( f3( std::declval< STATE >( ), std::declval< COST >( ) ) ) EVAL;
	struct element
	{
		STATE state;
		COST cost;
		EVAL eval;
		std::list< STATE > history;
		element( const STATE & s, const COST & c, const EVAL & e, const std::list< STATE > & h ) : state( s ), cost( c ), eval( e ), history( h ) { }
	};
	using namespace boost;
	using namespace multi_index;
	struct state_tag { };
	struct eval_tag { };
	multi_index_container
	<
		element,
		indexed_by
		<
			ordered_unique< tag< state_tag >, member< element, STATE, & element::state > >,
			ordered_non_unique< tag< eval_tag >, member< element, EVAL, & element::eval > >
		>
	> container;
	auto & state_index = container.get< state_tag >( );
	auto & eval_index = container.get< eval_tag >( );
	auto add_element = [&]( const STATE & s, const COST & c, const EVAL & e, const std::list< STATE > & h )
	{
		auto it = container.insert( element( s, c, e, h ) );
		if ( ! it->first )
		{
			if ( it->second.cost > c )
			{
				it->second.cost = c;
				it->second.e = f3( s, c );
				it->second.history = h;
			}
		}
	};
	auto add_parent = [&]( const element & child )
	{
		assert( ! child.history.empty( ) );
		const STATE & parent = child.history.back( );
		if ( state_index.count( parent ) == 0 )
		{
			std::list< STATE > parent_history = child.history;
			parent_history.pop_back( );
			COST reverse_cost;
			f1( child.state, boost::make_function_output_iterator(
					[&]( const std::pair< STATE, COST > & p )
					{
						if ( p.first == child.state )
						{ reverse_cost = p.second; }
					} ) );
			container.insert( element( parent, child.cost - reverse_cost, child.eval, parent_history ) );
		}
		else { state_index.modify( state_index.find( parent ), [&](element & e){ e.eval = std::max( e.eval, child.eval ); } ); }
	};
	add_element( inital_state, inital_cost, f3( inital_state, inital_cost ), { } );
	while ( ! container.empty( ) )
	{
		const element & current_element = eval_index.begin( )->second;
		if ( f2( current_element.state ) )
		{
			f4( current_element.cost );
			auto res = std::copy( current_element.history.begin( ), current_element.history.end( ), result );
			*res = current_element.state;
			++res;
			return res;
		}
		std::list< STATE > history = current_element.history;
		history.push_back( current_element.state );
		f1( current_element.state, boost::make_function_output_iterator(
				[&]( const std::pair< STATE, COST > & p )
				{
					add_element(
								p.first,
								p.second + current_element.cost,
								std::max( f3( p.first, p.second + current_element.cost, current_element.eval ) ),
								history );
				} ) );
		state_index.erase( current_element );
		while ( container.size( ) > container_limit )
		{
			const auto it = eval_index.rbegin( );
			const element & remove_element = it->second;
			if ( ! remove_element.history.empty( ) ) { add_parent( remove_element ); }
			eval_index.erase( it );
		}
	}
	return result;
}

template
<
	typename STATE, typename COST, typename EVAL,
	typename EXPAND, typename RETURN_IF, typename OUTITER, typename EVAL_FUNC, typename EVAL_OUTPUT
>
OUTITER iterative_best_first_search_helper(
		const STATE & inital_state,
		const COST & inital_cost,
		const EVAL & upper_bound,
		std::set< STATE > & history,
		EXPAND f1,
		RETURN_IF f2,
		EVAL_FUNC f3,
		EVAL_OUTPUT f4,
		OUTITER result )
{
	if ( f2( inital_state ) )
	{
		* result = inital_state;
		++result;
		return result;
	}
	EVAL inital_eval = f3( inital_state, inital_cost );
	if ( upper_bound < inital_eval )
	{
		f4( inital_eval );
		return result;
	}
	std::vector< std::pair< STATE, EVAL > > vec;
	f1( inital_state, std::back_inserter( vec ) );
	for ( const std::pair< STATE, EVAL > & s : vec )
	{
		if ( history.count( s.first ) != 0 ) { continue; }
		history.insert( s.first );
		bool find_solution = false;
		iterative_best_first_search_helper(
					s,
					inital_cost + s.second,
					upper_bound,
					history,
					f1,
					f2,
					f3,
					f4,
					boost::make_function_output_iterator(
						std::function< void( const STATE & ) >(
							[&]( const auto & state )
							{
								if ( ! find_solution )
								{
									* result = inital_state;
									++result;
									find_solution = true;
								}
								* result = state;
								++result;
							} ) ) );
		if ( find_solution ) { return result; }
		history.erase( s );
	}
	return result;
}

template
<
	typename STATE, typename COST, typename EVAL,
	typename EXPAND, typename RETURN_IF, typename OUTITER, typename EVAL_FUNC, typename EVAL_OUTPUT
>
OUTITER iterative_best_first_search(
		const STATE & inital_state,
		const COST & inital_cost,
		const EVAL & min_eval,
		const EVAL & upper_bound,
		EXPAND f1,
		RETURN_IF f2,
		EVAL_FUNC f3,
		OUTITER result )
{
	EVAL e;
	std::set< STATE > history;
	while ( e < min_eval )
	{
		iterative_best_first_search_helper(
					inital_state, inital_cost, upper_bound, history, f1, f2, f3, [&](const EVAL & ev){ e = std::max( e, ev ); }, result );
	}
}

#endif // SEARCH_HPP
