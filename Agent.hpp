#ifndef AGENT_HPP
#define AGENT_HPP
#include <functional>
#include <map>
#include <boost/optional/optional.hpp>

template< typename STATE, typename ACTION >
struct table_driven_agent
{
	std::map< STATE, ACTION > map;
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
#endif // AGENT_HPP
