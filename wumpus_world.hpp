#ifndef WUMPUS_WORLD_HPP
#define WUMPUS_WORLD_HPP
#include <list>
#include <cstddef>
#include <set>
#include <utility>
#include <vector>
#include <algorithm>
#include "../propositional_calculus_prover/CNF.hpp"
#include  "../propositional_calculus_prover/proposition.hpp"
#include <boost/function_output_iterator.hpp>
#include <boost/optional/optional.hpp>
#include "search.hpp"
#include "../propositional_calculus_prover/DPLL.hpp"
enum direction { north, east, south, west };
typedef std::pair< size_t, size_t > coordinate;
direction left( direction d )
{
	switch ( d )
	{
	case north:
		return east;
	case east:
		return south;
	case south:
		return west;
	case west:
		return north;
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
		if ( c.first < x - 1 ) { ++c.second; }
		break;
	case south:
		if ( c.second > 0 ) { --c.first; }
		break;
	case west:
		if ( c.first > 0 ) { --c.second; }
	}
	return c;
}
template< size_t x, size_t y >
struct wumpus_world
{
	enum action { turn_left, turn_right, move_forward, pickup_gold, shoot, climb };
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
	bool wumpus_killed;
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
		if ( c.second > 0 )
		{
			--c.first;
			out( );
			++c.first;
		}
		if ( c.first > 0 )
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
		if ( std::any_of( vec.begin( ), vec.end( ), [&]( const coordinate & c ){ return have_wumpus( c ); } ) ) { agent.current_sense.stench = true; }
		if ( std::any_of( vec.begin( ), vec.end( ), [&]( const coordinate & c ){ return pit.count( c ) != 0; } ) ) { agent.current_sense.breeze = true; }
	}
	int make_action( action act )
	{
		assert( ! is_end( ) );
		agent.current_sense = sense( );
		int ret = action_reward( );
		switch ( act )
		{
		case turn_right:
			agent.facing = right( agent.facing );
			break;
		case turn_left:
			agent.facing = left( agent.facing );
			break;
		case move_forward:
			coordinate tem = next_square( agent.position, agent.facing );
			if ( tem == agent.position ) { agent.current_sense.bump = true; }
			else { agent.position = tem; }
			if ( meet_wumpus( ) || fall_in_pit( ) ) { ret += killed_reward( ); }
			break;
		case pickup_gold:
			if ( agent.position == gold ) { agent.carrying_gold = true; }
			break;
		case shoot:
			if ( ! agent.have_arrow ) { break; }
			ret += use_arrow_reward( );
			agent.have_arrow = false;
			coordinate arrow = agent.position;
			do
			{
				coordinate tem = next_square( arrow, agent.facing );
				if ( tem == wumpus )
				{
					wumpus_killed = true;
					agent.current_sense.scream = true;
				}
			} while ( tem != arrow && [&](){arrow = tem;return true;}() );
			break;
		case climb:
			if ( agent.position == exit( ) )
			{
				agent.out_of_cave = true;
				if ( agent.carrying_gold ) { ret += gold_reward( ); }
			}
		}
		update_breeze_glitter_stench( );
		return ret;
	}
	wumpus_world( const coordinate & pos, direction dir, const coordinate & wum, const coordinate & gol, const std::set< coordinate > & pit ) : pit( pit ), gold( gol ), agent( pos, dir ), wumpus( wum ) { }
};

struct knoweldge_base
{
	typedef propositional_calculus::proposition proposition;
	typedef propositional_calculus::CNF CNF;
	propositional_calculus::CNF data;
	void insert( const propositional_calculus::clause & c )
	{
		data.data.insert( c );
		assert( propositional_calculus::DPLL( data ) );
	}
	void insert( const propositional_calculus::proposition & p )
	{
		CNF cnf( propositional_calculus::to_CNF( p ) );
		std::copy( cnf.data.begin( ), cnf.data.end( ), std::inserter( data.data, data.data.begin( ) ) );
		assert( propositional_calculus::DPLL( data ) );
	}
	bool certain( const proposition & p )
	{
		CNF cnf( propositional_calculus::to_CNF( propositional_calculus::make_not( p ) ) );
		for ( auto it = cnf.data.begin( ); it != cnf.data.end( ); ++it )
		{
			auto ret = data.data.insert( * it );
			if ( ! ret.second ) { it = cnf.data.erase( it ); }
		}
		bool ret = propositional_calculus::DPLL( data );
		for ( const propositional_calculus::clause & c : cnf.data ) { data.data.erase( c ); }
		return ! ret;
	}
	bool possible( const proposition & p )
	{
		CNF cnf( propositional_calculus::to_CNF( p ) );
		for ( auto it = cnf.data.begin( ); it != cnf.data.end( ); ++it )
		{
			auto ret = data.data.insert( * it );
			if ( ! ret.second ) { it = cnf.data.erase( it ); }
		}
		bool ret = propositional_calculus::DPLL( data );
		for ( const propositional_calculus::clause & c : cnf.data ) { data.data.erase( c ); }
		return ret;
	}
	knoweldge_base( ) : data( { } ) { }
};

template< size_t x, size_t y >
struct wumpus_agent
{
	typedef wumpus_world< x, y > world;
	knoweldge_base kb;
	const world & env;
	typedef propositional_calculus::proposition propsition;
	typedef typename world::action action;
	std::set< coordinate > un_visited;
	std::list< action > plan;
	typename world::action operator( )( )
	{
		un_visited.erase( env.agent.position );
		if ( env.agent.current_sense.scream )
		{
			std::set< literal > tem;
			for ( size_t i = 0; i < x; ++i )
			{
				for ( size_t j = 0; j < y; ++j )
				{ tem.insert( literal( wumpus( coordinate( i, j ) ), false ) ); }
			}
			kb.insert( tem );
		}
		else if( current_time != 0 )
		{
			for ( size_t i = 0; i < x; ++i )
			{
				for ( size_t j = 0; j < y; ++j )
				{ kb.insert( propositional_calculus::make_iff( propsition( wumpus( coordinate( i, j ) ) ), propsition( old_wumpus( coordinate( i, j ) ) ) ) ); }
			}
		}
		if ( env.agent.current_sense.breeze )
		{
			clause tem;
			env.surronding_squares( boost::make_function_output_iterator( [&]( const coordinate & c ){ tem.data.insert( literal( pit( c ), true ) ); } ) );
			kb.insert( tem );
		}
		else { env.surronding_squares( boost::make_function_output_iterator( [&]( const coordinate & c ){ kb.insert( { literal( pit( c ), false ) } ); } ) ); }
		if ( env.agent.current_sense.stench )
		{
			clause tem;
			env.surronding_squares( boost::make_function_output_iterator( [&]( const coordinate & c ){ tem.data.insert( literal( wumpus( c ), true ) ); } ) );
			kb.insert( tem );
		}
		else { env.surronding_squares( boost::make_function_output_iterator( [&]( const coordinate & c ){ kb.insert( { literal( wumpus( c ), false ) } ); } ) ); }
		kb.insert( { literal( wumpus( env.agent.position ), false ) } );
		kb.insert( { literal( pit( env.agent.position ), false ) } );
		if ( env.agent.current_sense.glitter ) { return world::pickup_gold; }
		std::set< coordinate > safe, possibly_safe, possible_wumpus;
		for ( size_t i = 0; i < x; ++i )
		{
			for ( size_t j = 0; j < x; ++j )
			{
				if ( kb.certain( propositional_calculus::make_not( propositional_calculus::make_or( wumpus( coordinate( i, j ) ), pit( coordinate( i, j ) ) ) ) ) )
				{
					safe.insert( coordinate( i, j ) );
					possibly_safe.insert( coordinate( i, j ) );
				}
				if ( kb.possible( propositional_calculus::make_not( propositional_calculus::make_or( wumpus( coordinate( i, j ) ), pit( coordinate( i, j ) ) ) ) ) )
				{ possibly_safe.insert( coordinate( i, j ) ); }
				if ( kb.possible( wumpus( coordinate( i, j ) ) ) ) { possible_wumpus.insert( coordinate( i, j ) );  }
			}
		}
		auto all_action =
				[]( const std::pair< coordinate, direction > &, auto it )
				{
					assert( world::action_reward( ) <= 0 );
					static std::vector< std::pair< action, size_t > > cop(
						{ { world::move_forward, -world::action_reward( ) },
						  { world::turn_left, -world::action_reward( ) },
						  { world::turn_right, -world::action_reward( ) } } );
					std::copy( cop.begin( ), cop.end( ), it );
				};
		auto next_state =
				[&]( auto allow_enter )
				{
					return
							[&]( std::pair< coordinate, direction > s, action act )
							{
								if ( act == world::turn_left ) { s.second = left( s.second ); }
								else if ( act == world::turn_right ) { s.second = right( s.second ); }
								else
								{
									assert( act == world::move_forward );
									if ( allow_enter( s.first ) ) { s.first = next_square< x, y >( s.first, s.second ); }
								}
								return s;
							};
				};
		auto eval_distance =
				[&](
					const std::pair< coordinate, direction > & source,
					const coordinate & dest )
		{ return std::abs( source.first.first - dest.first ) + std::abs( source.first.second - dest.second ); };
		std::pair< coordinate, direction > path_finder( env.agent.position, env.agent.facing );
		auto look_for_exit =
				[&]()
				{
					A_star( path_finder,
						static_cast< size_t >( 0 ),
						all_action,
						next_state( [&]( const coordinate & c ) { return safe.count( c ) != 0; } ),
						[&]( const decltype( path_finder ) & pf ) { return eval_distance( pf, env.exit( ) ); },
						[&]( const decltype( path_finder ) & pf ) { return pf.first == env.exit( ); },
						[](const size_t &){}, std::back_inserter( plan ) );
				};
		if ( plan.empty( ) && env.agent.carrying_gold ) { look_for_exit( ); }
		if ( plan.empty( ) )
		{
			A_star( path_finder,
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
					[&]( const decltype( path_finder ) & pf ) { return un_visited.count( pf.first ) != 0; },
					[](const size_t &){}, std::back_inserter( plan ) );
		}
		if ( plan.empty( ) && env.agent.have_arrow )
		{
			A_star( path_finder,
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
						//Situation( namely, a resonalble heuristic ) will be much complicated if we allow shoot from distance.
						//Also, an action does not cost much, and this example is not use to achive highest score,
						//	only to demonstrate the use of propositional inference engine.
					},
					[&]( const decltype( path_finder ) & pf ) { return un_visited.count( next_square< x, y >( pf.first, pf.second ) ) != 0; },
					[](const size_t &){}, std::back_inserter( plan ) );
			if ( ! plan.empty( ) ) { plan.push_back( world::shoot ); }
		}
		if ( plan.empty( ) )
		{
			A_star( path_finder,
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
					[&]( const decltype( path_finder ) & pf ) { return un_visited.count( pf.first ) != 0; },
					[](const size_t &){}, std::back_inserter( plan ) );
		}
		if ( plan.empty( ) ) { look_for_exit( ); }
		assert( ! plan.empty( ) );
		action ret = plan.front( );
		plan.pop_front( );
		return ret;
	}
	size_t current_time;
	typedef propositional_calculus::clause clause;
	typedef propositional_calculus::literal literal;
	std::string square( const coordinate & c ) const { return std::to_string( c.first ) + "," + std::to_string( c.second ); }
	std::string time( ) const { return std::to_string( current_time ); }
	std::string old_time( ) const
	{
		assert( current_time != 0 );
		return std::to_string( current_time - 1 );
	}
	std::string square_time( const coordinate & c ) const { return square( c ) + "," + time( ); }
	std::string old_square_time( const coordinate & c ) const { return square( c ) + "," + old_time( ); }
	std::string pit( const coordinate & c ) const { return "P" + square( c ); }
	std::string wumpus( const coordinate & c ) const { return "W" + square_time( c ); }
	std::string old_wumpus( const coordinate & c ) const { return "W" + old_square_time( c ); }
	wumpus_agent( const world & w ) : env( w ), current_time( 0 )
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
							for ( size_t l = ( k == i ) ? ( j + 1 ) : 0; l < 0; ++l )
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
		kb.insert( { literal( wumpus( w.agent.position ), false ) } );
		kb.insert( { literal( pit( w.agent.position ), false ) } );
	}
};

#endif // WUMPUS_WORLD_HPP
