#ifndef WUMPUS_WORLD_HPP
#define WUMPUS_WORLD_HPP
#include <list>
#include <cstddef>
#include <set>
#include <utility>
#include <vector>
#include <algorithm>
#include <boost/function_output_iterator.hpp>
#include "search.hpp"
#include "first_order_logic.hpp"
#include "sentence/CNF.hpp"
#include "SAT/DPLL.hpp"
namespace AI
{
    using namespace first_order_logic;
    enum direction { north, east, south, west };
    typedef std::pair< size_t, size_t > coordinate;
    direction left( direction d )
    {
        switch ( d )
        {
        case north:
            return west;
        case east:
            return north;
        case south:
            return east;
        case west:
            return south;
        }
        throw std::invalid_argument( "unknown direction" );
    }
    direction right( direction d ) { return left( left( left( d ) ) ); } //Two wrong wont make a right, but three left does!
    template< size_t x, size_t y >
    coordinate next_square( coordinate c, direction d )
    {
        switch ( d )
        {
        case north:
            if ( c.first < y - 1 ) { ++c.first; }
            break;
        case east:
            if ( c.second < x - 1 ) { ++c.second; }
            break;
        case south:
            if ( c.first > 0 ) { --c.first; }
            break;
        case west:
            if ( c.second > 0 ) { --c.second; }
        }
        return c;
    }
    template< size_t x, size_t y >
    struct wumpus_world
    {
        enum class action { turn_left, turn_right, move_forward, pickup_gold, shoot, climb };
        std::set< coordinate > pit;
        coordinate gold;
        struct sense { bool stench = false, breeze = false, glitter = false, bump = false, scream = false; };
        struct agent_t
        {
            coordinate position;
            direction facing;
            bool have_arrow = true;
            bool carrying_gold = false;
            bool out_of_cave = false;
            sense current_sense;
            agent_t( const coordinate & pos, direction dir ) : position( pos ), facing( dir ) { }
        } agent;
        coordinate wumpus;
        bool wumpus_killed = false;
        static const coordinate & exit( )
        {
            static coordinate ret( 0, 0 );
            return ret;
        }
        bool have_wumpus( const coordinate & c ) const { return wumpus_killed == false && c == wumpus; }
        bool meet_wumpus( ) const { return have_wumpus( agent.position ); }
        bool fall_in_pit( ) const { return pit.count( agent.position ) != 0; }
        bool is_end( ) const { return agent.out_of_cave || meet_wumpus( ) || fall_in_pit( ); }
        template< typename OUTITER >
        OUTITER surronding_squares( OUTITER result ) const
        {
            coordinate c = agent.position;
            auto out =
                    [&]( )
                    {
                        * result = c;
                        ++result;
                    };
            if ( c.first < y - 1 )
            {
                ++c.first;
                out( );
                --c.first;
            }
            if ( c.first < x - 1 )
            {
                ++c.second;
                out( );
                --c.second;
            }
            if ( c.first > 0 )
            {
                --c.first;
                out( );
                ++c.first;
            }
            if ( c.second > 0 )
            {
                --c.second;
                out( );
                ++c.second;
            }
            return result;
        }
        static int action_reward( ) { return -1; }
        static int killed_reward( ) { return -1000; }
        static int use_arrow_reward( ) { return -10; }
        static int gold_reward( ) { return 1000; }
        void update_breeze_glitter_stench( )
        {
            if ( ! agent.carrying_gold && agent.position == gold ) { agent.current_sense.glitter = true; }
            std::vector< coordinate > vec;
            surronding_squares( std::back_inserter( vec ) );
            if ( std::any_of( vec.begin( ), vec.end( ), [&]( const coordinate & c ){ return have_wumpus( c ); } ) )
            { agent.current_sense.stench = true; }
            if ( std::any_of( vec.begin( ), vec.end( ), [&]( const coordinate & c ){ return pit.count( c ) != 0; } ) )
            { agent.current_sense.breeze = true; }
        }
        int make_action( action act )
        {
            assert( ! is_end( ) );
            agent.current_sense = sense( );
            int ret = action_reward( );
            switch ( act )
            {
            case action::turn_right:
                agent.facing = right( agent.facing );
                break;
            case action::turn_left:
                agent.facing = left( agent.facing );
                break;
            case action::move_forward:
            {
                coordinate tem = next_square< x, y >( agent.position, agent.facing );
                if ( tem == agent.position ) { agent.current_sense.bump = true; }
                else { agent.position = tem; }
                if ( meet_wumpus( ) || fall_in_pit( ) ) { ret += killed_reward( ); }
                break;
            }
            case action::pickup_gold:
                if ( agent.position == gold ) { agent.carrying_gold = true; }
                break;
            case action::shoot:
            {
                if ( ! agent.have_arrow ) { break; }
                ret += use_arrow_reward( );
                agent.have_arrow = false;
                coordinate arrow = agent.position;
                while ( true )
                {
                    coordinate tem = next_square< x, y >( arrow, agent.facing );
                    if ( tem == wumpus )
                    {
                        wumpus_killed = true;
                        agent.current_sense.scream = true;
                    }
                    if ( tem == arrow ) { break; }
                    arrow = tem;
                }
                break;
            }
            case action::climb:
                if ( agent.position == exit( ) )
                {
                    agent.out_of_cave = true;
                    if ( agent.carrying_gold ) { ret += gold_reward( ); }
                }
            }
            update_breeze_glitter_stench( );
            return ret;
        }
        wumpus_world(
                const coordinate & pos, direction dir, const coordinate & wum, const coordinate & gol,
                const std::set< coordinate > & pit ) :
            pit( pit ), gold( gol ), agent( pos, dir ), wumpus( wum ) { }
    };

    struct knoweldge_base
    {
        typedef first_order_logic::free_propositional_sentence proposition;
        typedef std::set< std::set< first_order_logic::literal > > CNF;
        CNF data;
        void insert( const std::set< first_order_logic::literal > & c )
        {
            data.insert( c );
        }
        void insert( const proposition & p )
        {
            CNF cnf( first_order_logic::set_set_literal( p ) );
            std::copy( cnf.begin( ), cnf.end( ), std::inserter( data, data.begin( ) ) );
        }
        bool certain( const proposition & p )
        {
            CNF cnf( first_order_logic::set_set_literal( first_order_logic::make_not( p ) ) );
            for ( auto it = cnf.begin( ); it != cnf.end( ); )
            {
                auto ret = data.insert( * it );
                if ( ! ret.second ) { it = cnf.erase( it ); }
                else { ++it; }
            }
            bool ret = is_satisfiable( first_order_logic::DPLL( first_order_logic::set_set_to_list_list( data ) ) ).value( );
            for ( const auto & c : cnf ) { data.erase( c ); }
            return ! ret;
        }
        bool possible( const proposition & p ) { return ! certain( first_order_logic::make_not( p ) ); }
        knoweldge_base( ) : data( { } ) { }
    };

    template< size_t x, size_t y >
    struct wumpus_agent
    {
        typedef wumpus_world< x, y > world;
        knoweldge_base kb;
        const world & env;
        typedef first_order_logic::free_propositional_sentence proposition;
        typedef typename world::action action;
        std::set< coordinate > un_visited;
        std::list< action > plan;
        typename world::action operator( )( )
        {
            if ( ! env.wumpus_killed ) { kb.insert( { literal( wumpus( env.agent.position ), false ) } ); }
            kb.insert( { literal( pit( env.agent.position ), false ) } );
            un_visited.erase( env.agent.position );
            if ( env.agent.current_sense.breeze )
            {
                clause tem;
                env.surronding_squares( boost::make_function_output_iterator(
                    [&]( const coordinate & c ) { tem.insert( literal( pit( c ), true ) ); } ) );
                kb.insert( tem );
            }
            else { env.surronding_squares( boost::make_function_output_iterator(
                [&]( const coordinate & c ) { kb.insert( { literal( pit( c ), false ) } ); } ) ); }
            if ( ! env.wumpus_killed )
            {
                if ( env.agent.current_sense.stench )
                {
                    clause tem;
                    env.surronding_squares( boost::make_function_output_iterator(
                        [&]( const coordinate & c ){ tem.insert( literal( wumpus( c ), true ) ); } ) );
                    kb.insert( tem );
                }
                else
                {
                    env.surronding_squares( boost::make_function_output_iterator(
                        [&]( const coordinate & c ){ kb.insert( { literal( wumpus( c ), false ) } ); } ) );
                }
            }
            if ( env.agent.current_sense.glitter ) { return world::action::pickup_gold; }
            std::set< coordinate > safe, possibly_safe, possible_wumpus;
            for ( size_t i = 0; i < x; ++i )
            {
                for ( size_t j = 0; j < y; ++j )
                {
                    proposition safe_square =
                        env.wumpus_killed ?
                            first_order_logic::make_not( pit( coordinate( i, j ) ) ) :
                            first_order_logic::make_not(
                                first_order_logic::make_or(
                                    wumpus( coordinate( i, j ) ),
                                    pit( coordinate( i, j ) ) ) );
                    if ( kb.certain( safe_square ) )
                    {
                        safe.insert( coordinate( i, j ) );
                        possibly_safe.insert( coordinate( i, j ) );
                    }
                    else if ( kb.possible( safe_square ) )
                    { possibly_safe.insert( coordinate( i, j ) ); }
                    if ( ( ! env.wumpus_killed ) && kb.possible( wumpus( coordinate( i, j ) ) ) )
                    { possible_wumpus.insert( coordinate( i, j ) );  }
                }
            }
            auto all_action =
                []( const std::pair< coordinate, direction > &, auto it )
                {
                    assert( world::action_reward( ) <= 0 );
                    static std::vector< std::pair< action, int > > cop(
                        { { world::action::move_forward, -world::action_reward( ) },
                          { world::action::turn_left, -world::action_reward( ) },
                          { world::action::turn_right, -world::action_reward( ) } } );
                    std::copy( cop.begin( ), cop.end( ), it );
                };
            auto next_state =
                [&]( auto allow_enter )
                {
                    return
                        [&,allow_enter]( std::pair< coordinate, direction > s, action act )
                        {
                            if ( act == world::action::turn_left ) { s.second = left( s.second ); }
                            else if ( act == world::action::turn_right ) { s.second = right( s.second ); }
                            else
                            {
                                assert( act == world::action::move_forward );
                                if ( allow_enter( s.first ) )
                                { s.first = next_square< x, y >( s.first, s.second ); }
                            }
                            return s;
                        };
                };
            auto eval_distance =
                [&](
                    const std::pair< coordinate, direction > & source,
                    const coordinate & dest )
                {
                    return
                        std::abs(
                            static_cast< int >( source.first.first ) -
                            static_cast< int >( dest.first ) ) +
                        std::abs(
                            static_cast< int >( source.first.second ) -
                            static_cast< int >( dest.second ) );
                };
            std::pair< coordinate, direction > path_finder( env.agent.position, env.agent.facing );
            auto look_for_exit =
                    [&]( )
                    {
                        A_star< action >(
                            path_finder,
                            static_cast< size_t >( 0 ),
                            all_action,
                            next_state( [&]( const coordinate & c ) { return safe.count( c ) != 0; } ),
                            [&]( const decltype( path_finder ) & pf )
                            { return eval_distance( pf, env.exit( ) ); },
                            [&]( const decltype( path_finder ) & pf )
                            { return safe.count( pf.first ) != 0 && pf.first == env.exit( ); },
                            [](const size_t &){}, std::back_inserter( plan ) );
                            if ( ! plan.empty( ) ) { plan.push_back( world::action::climb ); }
                    };
            if ( plan.empty( ) && env.agent.carrying_gold ) { look_for_exit( ); }
            if ( plan.empty( ) )
            {
                A_star< action >(
                    path_finder,
                    static_cast< size_t >( 0 ),
                    all_action,
                    next_state( [&]( const coordinate & c ) { return safe.count( c ) != 0; } ),
                    [&]( const decltype( path_finder ) & pf )
                    {
                        assert( un_visited.size( ) != 0 );
                        return eval_distance(
                                pf,
                                * std::min_element(
                                    un_visited.begin( ),
                                    un_visited.end( ),
                                    [&]( const coordinate & l, const coordinate & r )
                                    { return eval_distance( pf, l ) < eval_distance( pf, r ); } ) );
                    },
                    [&]( const decltype( path_finder ) & pf )
                    { return safe.count( pf.first ) != 0 && un_visited.count( pf.first ) != 0; },
                    [](const size_t &){}, std::back_inserter( plan ) );
            }
            if ( plan.empty( ) && env.agent.have_arrow && ! env.wumpus_killed )
            {
                A_star< action >(
                    path_finder,
                    static_cast< size_t >( 0 ),
                    all_action,
                    next_state( [&]( const coordinate & c ) { return safe.count( c ) != 0; } ),
                    [&]( const decltype( path_finder ) & pf )
                    {
                        assert( un_visited.size( ) != 0 );
                        return eval_distance(
                                pf,
                                * std::min_element(
                                    possible_wumpus.begin( ),
                                    possible_wumpus.end( ),
                                    [&]( const coordinate & l, const coordinate & r )
                                    { return eval_distance( pf, l ) < eval_distance( pf, r ); } ) ) - 1;
                        //We'll shoot in front of wumpus.
                        //Situation( namely, a resonalble heuristic ) will be much complicated
                        //if we allow shoot from distance.
                        //Also, an action does not cost much, and this example is not use to
                        //achive highest score,
                        //only to demonstrate the use of propositional inference engine.
                    },
                    [&]( const decltype( path_finder ) & pf )
                    {
                        return safe.count( pf.first ) != 0 &&
                                un_visited.count( next_square< x, y >( pf.first, pf.second ) ) != 0;
                    },
                    [](const size_t &){}, std::back_inserter( plan ) );
                if ( ! plan.empty( ) ) { plan.push_back( world::action::shoot ); }
            }
            if ( plan.empty( ) )
            {
                A_star< action >(
                    path_finder,
                    static_cast< size_t >( 0 ),
                    all_action,
                    next_state( [&]( const coordinate & c ) { return possibly_safe.count( c ) != 0; } ),
                    [&]( const decltype( path_finder ) & pf )
                    {
                        assert( un_visited.size( ) != 0 );
                        return eval_distance(
                                pf,
                                * std::min_element(
                                    un_visited.begin( ),
                                    un_visited.end( ),
                                    [&]( const coordinate & l, const coordinate & r )
                                    { return eval_distance( pf, l ) < eval_distance( pf, r ); } ) );
                    },
                    [&]( const decltype( path_finder ) & pf )
                    { return possibly_safe.count( pf.first ) != 0 && un_visited.count( pf.first ) != 0; },
                    [](const size_t &){}, std::back_inserter( plan ) );
            }
            if ( plan.empty( ) ) { look_for_exit( ); }
            assert( ! plan.empty( ) );
            action ret = plan.front( );
            plan.pop_front( );
            return ret;
        }
        typedef first_order_logic::literal literal;
        typedef std::set< literal > clause;
        std::string square( const coordinate & c ) const
        { return std::to_string( c.first ) + "," + std::to_string( c.second ); }
        first_order_logic::atomic_sentence pit( const coordinate & c ) const { return first_order_logic::make_propositional_letter( "P" + square( c ) ); }
        first_order_logic::atomic_sentence wumpus( const coordinate & c ) const { return first_order_logic::make_propositional_letter( "W" + square( c ) ); }
        wumpus_agent( const world & w ) : env( w )
        {
            assert( w.is_end( ) == false );
            auto have_unique =
                    [&]( const auto & make )
                    {
                        {
                            std::set< literal > tem;
                            for ( size_t i = 0; i < x; ++i )
                            {
                                for ( size_t j = 0; j < y; ++j )
                                { tem.insert( literal( make( coordinate( i, j ) ), true ) ); }
                            }
                            kb.insert( clause( std::move( tem ) ) );
                        }
                        {
                            for ( size_t i = 0; i < x; ++i )
                            {
                                for ( size_t j = 0; j < y; ++j )
                                {
                                    for ( size_t k = i; k < x; ++k )
                                    {
                                        for ( size_t l = ( k == i ) ? ( j + 1 ) : 0; l < y; ++l )
                                        {
                                            std::set< literal > tem;
                                            tem.insert( literal( make( coordinate( i, j ) ), false ) );
                                            tem.insert( literal( make( coordinate( k, l ) ), false ) );
                                            kb.insert( clause( std::move( tem ) ) );
                                        }
                                    }
                                }
                            }
                        }
                    };
            for ( size_t i = 0; i < x; ++i )
            {
                for ( size_t j = 0; j < y; ++j )
                { un_visited.insert( ( coordinate( i, j ) ) ); }
            }
            have_unique( [this]( const coordinate & c ){ return wumpus( c ); } );
        }
    };
}
#endif // WUMPUS_WORLD_HPP
