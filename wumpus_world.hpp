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
enum direction { north, east, south, west };
template< size_t x, size_t y >
struct wumpus_world
{
	enum action { turn_left, turn_right, move_forward, pickup_gold, shoot, climb };
	typedef std::pair< size_t, size_t > coordinate;
	std::set< coordinate > pit;
	coordinate gold;
	struct sense { bool stench = false, breeze = false, glitter = false, bump = false, scream = false; };
	struct
	{
		coordinate position;
		direction facing;
		bool have_arrow;
		bool carrying_gold;
		bool out_of_cave;
		sense current_sense;
	} agent;
	coordinate wumpus;
	bool wumpus_killed;
	static coordinate exit( ) { return coordinate( 0, 0 ); }
	bool have_wumpus( const coordinate & c ) { return wumpus_killed == false && c == wumpus; }
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
			switch ( agent.facing )
			{
			case north:
				agent.facing = east;
				break;
			case east:
				agent.facing = south;
				break;
			case south:
				agent.facing = west;
				break;
			case west:
				agent.facing = north;
			}
			break;
		case turn_left:
			switch ( agent.facing )
			{
			case north:
				agent.facing = west;
				break;
			case east:
				agent.facing = north;
				break;
			case south:
				agent.facing = east;
				break;
			case west:
				agent.facing = south;
			}
			break;
		case move_forward:
			switch ( agent.facing )
			{
			case north:
				if ( agent.position.first < y - 1 ) { ++agent.position.first; }
				else { agent.current_sense.bump = true; }
				break;
			case east:
				if ( agent.position.first < x - 1 ) { ++agent.position.second; }
				else { agent.current_sense.bump = true; }
				break;
			case south:
				if ( agent.position.second > 0 ) { --agent.position.first; }
				else { agent.current_sense.bump = true; }
				break;
			case west:
				if ( agent.position.first > 0 ) { --agent.position.second; }
				else { agent.current_sense.bump = true; }
			}
			if ( meet_wumpus( ) || fall_in_pit( ) ) { ret += killed_reward( ); }
			break;
		case pickup_gold:
			if ( agent.position == gold ) { agent.carrying_gold = true; }
			break;
		case shoot:
			if ( ! agent.have_arrow ) { break; }
			ret += use_arrow_reward( );
			agent.have_arrow = false;
			coordinate arrow;
			switch ( agent.facing )
			{
			case north:
				while ( arrow.first < y - 1 )
				{
					++arrow.first;
					if ( arrow == wumpus ) { goto http; }
				}
				break;
			case east:
				while ( arrow.first < x - 1 )
				{
					++arrow.second;
					if ( arrow == wumpus ) { goto http; }
				}
				break;
			case south:
				while ( arrow.second > 0 )
				{
					--arrow.first;
					if ( arrow == wumpus ) { goto http; }
				}
				break;
			case west:
				while ( arrow.first > 0 )
				{
					--arrow.second;
					if ( arrow == wumpus ) { goto http; }
				}
			}
			break;
			http://marisa.moe
			wumpus_killed = true;
			agent.current_sense.scream = true;
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
};

struct knoweldge_base
{
	propositional_calculus::CNF data;
	void insert( const propositional_calculus::clause & c ) { data.data.insert( c ); }
	void insert( const propositional_calculus::proposition & p )
	{
		propositional_calculus::CNF cnf( propositional_calculus::to_CNF( p ) );
		std::copy( cnf.data.begin( ), cnf.data.end( ), std::inserter( data.data, data.data.begin( ) ) );
	}
};

template< size_t x, size_t y >
struct wumpus_agent
{
	typedef wumpus_world< x, y > world;
	knoweldge_base kb;
	const world & env;
	typedef propositional_calculus::proposition propsition;
	typedef typename world::action action;
	typedef typename world::coordinate coordinate;
	void update_previous_action( const action & act ) { }
	std::set< coordinate > un_visited;
	std::list< action > plan;
	typename world::action operator( )( const boost::optional< action > & act )
	{
		un_visited.erase( env.agent.position );
		if ( act ) { update_previous_action( act ); }
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
				{ kb.insert( propositional_calculus::make_iff( proposition( wumpus( coordinate( i, j ) ), propsition( old_wumpus( coordinate( i, j ) ) ) ) ) ); }
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
		if ( ! plan.empty( ) && env.agent.carrying_gold )
		{
			//plan a route out
		}
		if ( ! plan.empty( ) )
		{
			//plan to go to a safe square to explore
		}
		if ( ! plan.empty( ) && env.agent.have_arrow )
		{
			//plan to shoot wumpus
		}
		if ( ! plan.empty( ) )
		{
			//plan to go to a possibly safe square to explore
		}
		if ( ! plan.empty( ) )
		{
			//plan a route out
		}
		if ( ! plan.empty( ) )
		{
			action ret = plan.front( );
			plan.pop_front( );
			return ret;
		}
	}
	size_t current_time;
	typedef propositional_calculus::clause clause;
	typedef propositional_calculus::literal literal;
	std::string square( const coordinate & c ) const { return std::to_string( c.first ) + "," + std::to_string( c.second ); }
	std::string time( ) const { return std::string( current_time ); }
	std::string old_time( ) const
	{
		assert( current_time != 0 );
		return std::string( current_time - 1 );
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
								tem.insert( literal( make( i, j ), false ) );
								tem.insert( literal( make( k, l ), false ) );
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
		have_unique( [this]( const typename world::coordinate & c ){ return gold( c ); } );
		have_unique( [this]( const typename world::coordinate & c ){ return wumpus( c ); } );
		kb.insert( { literal( wumpus( w.agent.position ), false ) } );
		kb.insert( { literal( pit( w.agent.position ), false ) } );
	}
};

#endif // WUMPUS_WORLD_HPP
