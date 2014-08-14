#ifndef AGENT_HPP
#define AGENT_HPP
#include <functional>
#include <map>
#include <boost/optional/optional.hpp>
#include <set>
#include "search.hpp"
template< typename STATE, typename ACTION >
struct table_driven_agent
{
	std::map< STATE, ACTION > map;
	table_driven_agent( const std::map< STATE, ACTION > & map ) : map( map ) { }
	boost::optional< ACTION > operator ( )( const STATE & s ) const
	{
		auto res = map.find( s );
		if ( res != map.end( ) ) { return res->second; }
		return boost::optional< ACTION >( );
	}

	template< typename IT, typename ON_COLLID >
	void add_entry( IT begin, IT end, ON_COLLID handler )
	{
		while ( begin != end )
		{
			auto res = map.insert( * begin );
			if ( ! res.second ) { handler( * begin, res.first->second ); }
			++begin;
		}
	}

	template< typename IT >
	void add_entry( IT begin, IT end ) { map.insert( begin, end ); }

	template< typename IT >
	void remove_entry( IT begin, IT end ) { map.erase( begin, end ); }
};

template< typename STATE, typename ACTION, typename PRIOREITY_TYPE = std::size_t >
struct simple_reflex_agent
{
	std::map< PRIOREITY_TYPE, std::function< boost::optional< ACTION >( const STATE & ) > > map;
	boost::optional< ACTION > operator ( )( const STATE & s ) const
	{
		for ( const auto & i : map )
		{
			auto res = i.second( s );
			if ( res ) { return res; }
		}
		return boost::optional< ACTION >( );
	}
	boost::optional< ACTION > operator ( )( const STATE & s )
	{
		for ( auto & i : map )
		{
			auto res = i.second( s );
			if ( res ) { return res; }
		}
		return boost::optional< ACTION >( );
	}
	template< typename IT, typename ON_COLLID >
	void add_entry( IT begin, IT end, ON_COLLID handler )
	{
		while ( begin != end )
		{
			auto res = map.insert( * begin );
			if ( ! res.second ) { handler( * begin, res.first->second ); }
			++begin;
		}
	}

	template< typename IT >
	void add_entry( IT begin, IT end ) { map.insert( begin, end ); }

	template< typename IT >
	void remove_entry( IT begin, IT end ) { map.erase( begin, end ); }

};

template< typename STATE, typename ACTION, typename ACTION_GENERATOR, typename RANDOM_DEVICE >
struct random_walk_agent
{
	ACTION_GENERATOR act;
	mutable RANDOM_DEVICE rd;
	boost::optional< ACTION > operator( )( const STATE & s ) const
	{
		std::vector< ACTION > vec;
		act( s, std::back_inserter( vec ) );
		if ( vec.empty( ) ) { return boost::optional< ACTION >( ); }
		std::uniform_int_distribution<> uid( 0, vec.size( ) - 1 );
		return vec[uid( rd )];
	}
};

template< typename STATE, typename ACTION, typename ACTION_GENERATOR, typename NEXT_STATE, typename GOAL_TEST, typename RANDOM_DEVICE >
struct online_DFS_agent
{
	ACTION_GENERATOR act;
	NEXT_STATE ns;
	GOAL_TEST gt;
	mutable RANDOM_DEVICE rd;
	std::map< STATE, std::map< ACTION, STATE > > map;
	std::multimap< STATE, ACTION > untried;
	bool operator( )( const STATE & s )
	{
		STATE state = s;
		while ( ! gt( state ) )
		{
			std::vector< ACTION > vec;
			act( state, std::back_inserter( vec ) );
			if ( vec.empty( ) ) { return false; }
			bool made_act = false;
			if ( map.count( state ) == 0 ) { map.insert( { state, std::map< ACTION, STATE >( ) } ); }
			std::map< ACTION, STATE > & m = map.find( state )->second;
			for ( const ACTION a : vec )
			{
				if ( m.count( a ) == 0 )
				{
					if ( made_act ) { untried.insert( std::make_pair( state, a ) ); }
					else
					{
						STATE tem = ns( state, a );
						m.insert( a, tem );
						std::swap( tem, state );
						made_act = true;
					}
				}
			}
			if ( ! made_act )
			{
				std::vector< ACTION > vec;
				depth_first_search(
							s,
							[&]( const STATE & state, auto it )
							{
								assert( map.count( state ) != 0 );
								std::map< ACTION, STATE > & m = map.find( state )->second;
								auto tran = [](const std::pair< ACTION, STATE > & p){ return p.first; };
								std::copy( boost::make_transform_iterator( map.begin( ), tran ), boost::make_transform_iterator( map.begin( ), tran ), it );
							},
							[&]( const STATE & state, const ACTION & action )
							{
								assert( map.count( state ) != 0 );
								std::map< ACTION, STATE > & m = map.find( state )->second;
								assert( m.count( action ) != 0 );
								return m.find( action )->second;
							},
							[&](const STATE & st){ return untried.count( st ) != 0; },
							std::back_inserter( vec ) );
				for ( const ACTION & a : vec ) { state = ns( state, a ); }
			}
		}
	}
};
#endif // AGENT_HPP
