#ifndef TEST_HPP
#define TEST_HPP
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <utility>
#include <functional>
#include "agent.hpp"
#include "CSP.hpp"
#include "search.hpp"
#include "wumpus_world.hpp"
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
				std::copy(
							boost::make_transform_iterator( tem.first, sf ),
							boost::make_transform_iterator( tem.second, sf ),
							it );
			},
			[](location, location ret){return ret;},
			[](location l){ return l == Bucharest; },
			std::back_inserter( res ) );
	BOOST_CHECK_EQUAL( res, std::list< location >( { Fagaras, Bucharest } ) );
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
			[](location, location ret){return ret;},
			[](location l){ return l == Bucharest; },
			std::back_inserter( res ) );
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
			[](location, location ret){ return ret; },
			[](location l){ return l == Bucharest; },
			std::back_inserter( res ) );
	BOOST_CHECK_EQUAL( res, std::list< location >( { Fagaras, Bucharest } ) );
}

BOOST_AUTO_TEST_CASE( BBFS )
{
	auto sf = []( const std::pair< location, std::pair< location, size_t > > & pp ){ return pp.second.first; };
	auto expand = [&]( location l, const auto & it )
	{
		auto tem = map( ).equal_range( l );
		std::copy(
					boost::make_transform_iterator( tem.first, sf ),
					boost::make_transform_iterator( tem.second, sf ),
					it );
	};
	std::list< location > res;
	biderectional_breadth_first_search( Lugoj, Fagaras, expand, expand, std::back_inserter( res ) );
	BOOST_CHECK_EQUAL( res, std::list< location >( { Lugoj, Timisoara, Arad, Sibiu, Fagaras } ) );
}
struct vacum_world
{
	bool is_left = true;
	bool left_clean = false;
	bool right_clean = false;
	vacum_world( bool i, bool l, bool r ) : is_left( i ), left_clean( l ), right_clean( r ) { }
	bool operator < ( const vacum_world & comp ) const { return std::tie( is_left, left_clean, right_clean ) < std::tie( comp.is_left, comp.left_clean, comp.right_clean ); }
};
enum class vacum_action { left, right, suck };
typedef std::map< vacum_world, vacum_action > ignore1;
BOOST_TEST_DONT_PRINT_LOG_VALUE( ignore1 );
BOOST_AUTO_TEST_CASE( AOS )
{
	std::list< vacum_action > act{ vacum_action::left, vacum_action::right, vacum_action::suck };
	auto possibility = [&]( const vacum_world & v, vacum_action a, auto it)
	{
		std::vector< vacum_world > vec;
		switch ( a )
		{
			case vacum_action::left:
			vec.push_back( v );
			{
				vacum_world n( v );
				n.is_left = true;
				vec.push_back( n );
			}
			break;
			case vacum_action::right:
			vec.push_back( v );
			{
				vacum_world n( v );
				n.is_left = false;
				vec.push_back( n );
			}
			break;
			case vacum_action::suck:
			{
				vacum_world n( v );
				( n.is_left ? n.left_clean : n.right_clean ) = true;
				vec.push_back( n );
			}
			break;
		}
		std::copy( vec.begin( ), vec.end( ), it );
	};
	auto cleaned = [](const vacum_world & v){return v.left_clean && v.right_clean; };
	auto res = and_or_search< vacum_action >(
			vacum_world( true, false, false ),
			cleaned,
			[&](const vacum_world &, auto it){std::copy( act.begin( ), act.end( ), it );},
			possibility );
	BOOST_CHECK( res );
	table_driven_agent< vacum_world, vacum_action > cleaner( * res );
	auto test = [&](const auto & self,const vacum_world & v, std::set< vacum_world > & history)
	{
		if ( cleaned( v ) ) { return true; }
		auto act = cleaner( v );
		assert( act );
		auto action = * act;
		std::vector< vacum_world > ns;
		possibility( v, action, std::back_inserter( ns ) );
		bool dead_end = true;
		for ( const vacum_world & vw : ns )
		{
			if ( history.count( vw ) != 0 ) { continue; }
			history.insert( vw );
			dead_end = false;
			if ( ! self( self, vw, history ) ) { return false; }
			history.erase( vw );
		}
		return ! dead_end;
	};
	std::set< vacum_world > h;
	BOOST_CHECK( test( test, vacum_world( false, false, false ), h ) );
}

BOOST_AUTO_TEST_CASE( CSP )
{
	{
		enum var { F, T, U, W, R, O, C1, C2, C3 };
		std::vector< std::map< var, size_t > > result;
		std::list< size_t > digits { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
		std::list< size_t > carry { 0, 1 };
		std::list< local_constraint< var, size_t > > con;
		con.push_back(
				make_local_constraint< var, size_t >(
					std::vector< var >( { O, R, C1 } ),
					[&]( const std::vector< std::reference_wrapper< const size_t > > & ass )
					{ return ass[0] * 2 == ass[1] + ass[2] * 10; } ) );
		con.push_back(
				make_local_constraint< var, size_t >(
					std::vector< var >( { O, R } ),
					[&]( const std::vector< std::reference_wrapper< const size_t > > & ass )
					{ return ( ass[0] * 2 - ass[1] ) % 10 == 0; } ) );
		con.push_back(
				make_local_constraint< var, size_t >(
					std::vector< var >( { C1, W, U, C2 } ),
					[&]( const std::vector< std::reference_wrapper< const size_t > > & ass )
					{ return ass[0] + ass[1] * 2 == ass[2] + 10 * ass[3]; } ) );
		con.push_back(
				make_local_constraint< var, size_t >(
					std::vector< var >( { C1, W, U } ),
					[&]( const std::vector< std::reference_wrapper< const size_t > > & ass )
					{ return ( ass[0] + ass[1] * 2 - ass[2] ) % 10 == 0; } ) );
		con.push_back(
				make_local_constraint< var, size_t >(
					std::vector< var >( { C2, T, O, C3 } ),
					[&]( const std::vector< std::reference_wrapper< const size_t > > & ass )
					{ return ass[0] + ass[1] * 2 == ass[2] + 10 * ass[3]; } ) );
		con.push_back(
				make_local_constraint< var, size_t >(
					std::vector< var >( { C2, T, O } ),
					[&]( const std::vector< std::reference_wrapper< const size_t > > & ass )
					{ return ( ass[0] + ass[1] * 2 - ass[2] ) % 10 == 0; } ) );
		con.push_back(
				make_local_constraint< var, size_t >(
					std::vector< var >( { C3, F } ),
					[&]( const std::vector< std::reference_wrapper< const size_t > > & ass )
					{ return ass[0] == ass[1]; } ) );
		con.push_back(
				make_local_constraint< var, size_t >(
					std::vector< var >( { F } ),
					[&]( const std::vector< std::reference_wrapper< const size_t > > & ass )
					{ return ass[0] != 0; } ) );
		for ( var i : { F, T, U, W, R, O } )
		{
			for ( var j : { F, T, U, W, R, O } )
			{
				if ( i > j )
				{
					con.push_back(
							make_local_constraint< var, size_t >(
								std::vector< var >( { i, j } ),
								[&]( const std::vector< std::reference_wrapper< const size_t > > & ass )
								{ return ass[0] != ass[1]; } ) );
				}
			}
		}
		backtracking_search(
				std::map< var, std::list< size_t > >( { {F, digits},{O, digits},{U, digits},{R, digits},{T, digits},{W, digits},{C1, carry},{C2, carry},{C3, carry} } ),
				4,
				con,
				std::back_inserter( result ) );
		BOOST_CHECK_EQUAL( result.size( ), 7 );
		for ( const std::map< var, size_t > & ass : result )
		{
			for ( const auto & c : con )
			{ BOOST_CHECK( c( ass ) ); }
		}
	}
	{
		enum var { A = 0, B = 1, C = 2, D = 3 };
		std::vector< std::map< size_t, var > > result;
		std::list< var > answers { A, B, C, D };
		std::list< local_constraint< size_t, var > > con;
		auto make_equal =
				[&]( var v, size_t s, size_t l, size_t r )
				{
					con.push_back( make_local_constraint< size_t, var >
					(
						{ s, l, r },
						[=]( const std::vector< std::reference_wrapper< const var > > & vec ){ return vec[0] != v || vec[1] == vec[2]; } )
					);
				};
		auto make_constrain_2 = [&]( var a, var b )
		{
			con.push_back( make_local_constraint< size_t, var >
			(
				{ 2, 5 },
				[=]( const std::vector< std::reference_wrapper< const var > > & vec ){ return vec[0] != a || vec[1] == b; }
			) );
		};
		make_constrain_2( A, C );
		make_constrain_2( B, D );
		make_constrain_2( C, A );
		make_constrain_2( D, B );
		auto make_constrain_3 =
				[&]( var v, size_t x )
		{
			con.push_back( make_local_constraint< size_t, var >
			(
				{ 3, 6, 2, 4 },
				[=]( const std::vector< std::reference_wrapper< const var > > & vec )
				{
					return vec[0] != v ||
							( vec[x] != vec[1] && vec[x] != vec[2] && vec[x] != vec[3] );
				}
			) );
		};
		make_constrain_3( A, 0 );
		make_constrain_3( B, 1 );
		make_constrain_3( C, 2 );
		make_constrain_3( D, 3 );
		make_equal( A, 5, 5, 8 );
		make_equal( B, 5, 5, 4 );
		make_equal( C, 5, 5, 9 );
		make_equal( D, 5, 5, 7 );
		make_equal( A, 4, 1, 5 );
		make_equal( B, 4, 2, 7 );
		make_equal( C, 4, 1, 9 );
		make_equal( D, 4, 6, 10 );
		make_equal( A, 6, 2, 4 );
		make_equal( B, 6, 1, 6 );
		make_equal( C, 6, 3, 10 );
		make_equal( D, 6, 5, 9 );
		make_equal( A, 6, 2, 8 );
		make_equal( B, 6, 1, 8 );
		make_equal( C, 6, 3, 8 );
		make_equal( D, 6, 5, 8 );
		auto make_min = [&]( var answer, var min_count )
		{
			con.push_back( make_local_constraint< size_t, var >
			(
				{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 },
				[=]( const std::vector< std::reference_wrapper< const var > > & vec )
				{
					size_t
							A_count = std::count( vec.begin( ), vec.end( ), A ),
							B_count = std::count( vec.begin( ), vec.end( ), B ),
							C_count = std::count( vec.begin( ), vec.end( ), C ),
							D_count = std::count( vec.begin( ), vec.end( ), D );
					return vec[6] != answer ||
							( ( min_count == A ?
									A_count :
									( min_count == B ?
										B_count :
										( min_count == C ?
											C_count : D_count ) ) ) ==
							std::min( std::min( A_count, B_count ), std::min( C_count, D_count ) ) );
				}
			) );
		};
		auto make_minmax = [&]( var answer, size_t difference )
		{
			con.push_back( make_local_constraint< size_t, var >
			(
				{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 },
				[=]( const std::vector< std::reference_wrapper< const var > > & vec )
				{
					size_t
							A_count = std::count( vec.begin( ), vec.end( ), A ),
							B_count = std::count( vec.begin( ), vec.end( ), B ),
							C_count = std::count( vec.begin( ), vec.end( ), C ),
							D_count = std::count( vec.begin( ), vec.end( ), D );
					return vec[9] != answer ||
							( ( std::max( std::max( A_count, B_count ), std::max( C_count, D_count ) ) -
							  std::min( std::min( A_count, B_count ), std::min( C_count, D_count ) ) ) ==
							difference );
				}
			) );
		};
		auto non_adjacent =
				[]( var a, var b )
				{
					switch ( a )
					{
						case A:
							return b == D || b == C;
						case B:
							return b == D;
						case C:
							return b == A;
						case D:
							return b == A || b == B;
					}
					throw std::invalid_argument( "unknown enum value." );
				};
		auto make_constrain_8 =
				[&]( var answer, size_t question )
		{
			con.push_back( make_local_constraint< size_t, var >
			(
				{ 8, 1, question },
				[=]( const std::vector< std::reference_wrapper< const var > > & vec )
				{ return vec[0] != answer || non_adjacent( vec[1], vec[2] ); }
			) );
		};
		auto make_constrain_9 =
				[&]( var answer, size_t question )
		{
			con.push_back( make_local_constraint< size_t, var >
			(
				{ 9, 1, 6, question, 5 },
				[=]( const std::vector< std::reference_wrapper< const var > > & vec )
				{ return vec[0] != answer || ( ( vec[1] == vec[2] ) != ( vec[3] == vec[4] ) ); }
			) );
		};
		make_constrain_8( A, 7 );
		make_constrain_8( B, 5 );
		make_constrain_8( C, 2 );
		make_constrain_8( D, 10 );
		make_constrain_9( A, 6 );
		make_constrain_9( B, 10 );
		make_constrain_9( C, 2 );
		make_constrain_9( D, 9 );
		make_min( A, C );
		make_min( B, B );
		make_min( C, A );
		make_min( D, D );
		make_minmax( A, 3 );
		make_minmax( B, 2 );
		make_minmax( C, 4 );
		make_minmax( D, 1 );
		backtracking_search(
				std::map< size_t, std::list< var > >(
					{
						{1, answers},
						{2, answers},
						{3, answers},
						{4, answers},
						{5, answers},
						{6, answers},
						{7, answers},
						{8, answers},
						{9, answers},
						{10, answers}
					} ),
				10,
				con,
				std::back_inserter( result ) );
		for ( const std::map< size_t, var > & ass : result )
		{
			for ( const auto & p : con )
			{ BOOST_CHECK( p( ass ) ); }
		}
		BOOST_CHECK( result.size( ) == 1 );
	}
}

BOOST_AUTO_TEST_CASE( ISSUE_2 )
{
	std::vector< size_t > res;
	breadth_first_search( 0, [](size_t i, auto it){ * it = ( i + 1 ) % 7;++it; },[](size_t, size_t ret){ return ret; }, [](size_t i){ return i == 42; }, std::back_inserter( res ) );
	BOOST_CHECK( res.empty( ) );
}

BOOST_AUTO_TEST_CASE( INFERENCE_AGENT )
{
	wumpus_world< 4, 4 > ww( coordinate( 0, 0 ), east, coordinate( 2, 0 ), coordinate( 2, 1 ), { coordinate( 0, 2 ), coordinate( 2, 2 ), coordinate( 3, 3 ) } );
	wumpus_agent< 4, 4 > agent( ww );
	int score = 0;
	while ( ! ww.is_end( ) )
	{
		auto act = agent( );
		score += ww.make_action( act );
	}
	BOOST_CHECK_GT( score, 0 );
}

#endif // TEST_HPP
