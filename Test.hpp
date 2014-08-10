#ifndef TEST_HPP
#define TEST_HPP
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include "Search.hpp"
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
#endif // TEST_HPP
