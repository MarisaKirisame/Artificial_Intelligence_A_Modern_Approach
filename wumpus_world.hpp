#ifndef WUMPUS_WORLD_HPP
#define WUMPUS_WORLD_HPP
#include <cstddef>
#include <set>
#include <utility>
#include <vector>
#include <algorithm>
#include "../propositional_calculus_prover/CNF.hpp"
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
	coordinate exit;
	bool meet_wumpus( ) const { return ( wumpus_killed == false && agent.position == wumpus ); }
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
	int make_action( action act )
	{
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
			if ( agent.position == exit )
			{
				agent.out_of_cave = true;
				if ( agent.carrying_gold ) { ret += gold_reward( ); }
			}
		}
		if ( ! agent.carrying_gold && agent.position == gold ) { agent.current_sense.glitter = true; }
		std::vector< coordinate > vec;
		surronding_squares( std::back_inserter( vec ) );
		if ( std::any_of( vec.begin( ), vec.end( ), [&]( const coordinate & c ){ return c == wumpus; } ) ) { agent.current_sense.stench = true; }
		if ( std::any_of( vec.begin( ), vec.end( ), [&]( const coordinate & c ){ return pit.count( c ) != 0; } ) ) { agent.current_sense.breeze = true; }
		return ret;
	}
};

template< size_t x, size_t y >
struct wumpus_agent
{
	typedef wumpus_world< x, y > world;
	propositional_calculus::CNF knoweldge_base;
	const world & env;
	typename world::action operator( )( const typename world::sense & ) { }
};

#endif // WUMPUS_WORLD_HPP
