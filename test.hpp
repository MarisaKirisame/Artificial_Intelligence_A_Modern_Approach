#ifndef TEST_HPP
#define TEST_HPP
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include <utility>
#include <functional>
#include "agent.hpp"
#include "search.hpp"
#include "wumpus_world.hpp"
using namespace AI;
enum location
{
    Sibiu, Fagaras, Bucharest, Pitesti, Rimnicu_Vilcea,
    Oradea, Zerind, Arad, Timisoara, Lugoj,
    Mehadia, Drobeta, Craivoa, Giurgiu, Urziceni,
    Hirsova, Eforie, Vaslui, Iasi, Neamt
};

const std::multimap< location, std::pair< location, size_t > > & map( )
{
    static std::multimap< location, std::pair< location, size_t > > ret(
        []( )
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
    auto sf =
        []( const std::pair< location, std::pair< location, size_t > > & pp )
        { return pp.second.first; };
    std::list< location > res;
    breadth_first_search< location >(
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
            std::copy(
                boost::make_transform_iterator( tem.first, sf ),
                boost::make_transform_iterator( tem.second, sf ),
                it );
        },
        [](location l){ return l == Bucharest; },
        []( auto cost ){ BOOST_CHECK_EQUAL( cost, 278 ); },
        std::back_inserter( res ) );
    BOOST_CHECK_EQUAL( res, std::list< location >( { Sibiu, Rimnicu_Vilcea, Pitesti, Bucharest } ) );
}

BOOST_AUTO_TEST_CASE( DFS )
{
    auto sf =
        []( const std::pair< location, std::pair< location, size_t > > & pp )
        { return pp.second.first; };
    std::list< location > res;
    depth_first_search< location >(
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
    BOOST_CHECK_EQUAL( res.back( ), Bucharest );
}

BOOST_AUTO_TEST_CASE( IDDFS )
{
    auto sf =
        []( const std::pair< location, std::pair< location, size_t > > & pp )
        { return pp.second.first; };
    std::list< location > res;
    iterative_deepening_depth_first_search< location >(
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
    auto sf =
        []( const std::pair< location, std::pair< location, size_t > > & pp )
        { return pp.second.first; };
    auto expand = [&]( location l, const auto & it )
    {
        auto tem = map( ).equal_range( l );
        std::copy(
            boost::make_transform_iterator( tem.first, sf ),
            boost::make_transform_iterator( tem.second, sf ),
            it );
    };
    std::list< location > res, rev;
    biderectional_breadth_first_search< location, location >(
        Lugoj,
        Fagaras,
        expand,
        []( location, location s ) { return s; },
        std::back_inserter( res ),
        expand,
            []( location, location s ) { return s; },
            std::back_inserter( rev ) );
            std::copy( rev.rbegin( ), rev.rend( ), std::back_insert_iterator< decltype( res ) >( res ) );
    BOOST_CHECK_EQUAL( res, std::list< location >( { Timisoara, Arad, Arad, Sibiu } ) );
}

struct vacum_world
{
    bool is_left = true;
    bool left_clean = false;
    bool right_clean = false;
    vacum_world( bool i, bool l, bool r ) : is_left( i ), left_clean( l ), right_clean( r ) { }
    bool operator < ( const vacum_world & comp ) const
    {
        return
            std::tie( is_left, left_clean, right_clean ) <
            std::tie( comp.is_left, comp.left_clean, comp.right_clean );
    }
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
    std::set< vacum_world > history;
    auto test = common::fix(
        [&]( auto & self, const vacum_world & v ) -> bool
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
                if ( ! self( vw ) ) { return false; }
                history.erase( vw );
            }
            return ! dead_end;
        } );
    BOOST_CHECK( test( vacum_world( false, false, false ) ) );
}

BOOST_AUTO_TEST_CASE( ISSUE_2 )
{
    std::vector< size_t > res;
    breadth_first_search< size_t >(
        0,
        [](size_t i, auto it) { * it = ( i + 1 ) % 7; ++it; },
        [](size_t, size_t ret) { return ret; },
        [](size_t i){ return i == 42; },
        std::back_inserter( res ) );
    BOOST_CHECK( res.empty( ) );
}

BOOST_AUTO_TEST_CASE( INFERENCE_AGENT )
{
    wumpus_world< 4, 4 > ww(
        coordinate( 0, 0 ),
        east,
        coordinate( 2, 0 ),
        coordinate( 2, 1 ),
        { coordinate( 0, 2 ), coordinate( 2, 2 ), coordinate( 3, 3 ) } );
    wumpus_agent< 4, 4 > agent( ww );
    int score = 0;
    while ( ! ww.is_end( ) )
    {
        auto act = agent( );
        score += ww.make_action( act );
    }
    BOOST_CHECK_GT( score, 0 );
}

namespace boost { namespace unit_test { namespace ut_detail {
    std::string normalize_test_case_name(const_string name) {
        return ( name[0] == '&' ? std::string(name.begin()+1, name.size()-1) : std::string(name.begin(), name.size() )); }}}}

#endif // TEST_HPP
