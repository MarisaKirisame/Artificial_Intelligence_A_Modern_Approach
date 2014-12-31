#ifndef CSP_HPP
#define CSP_HPP
#include <set>
#include <vector>
#include <cassert>
#include <algorithm>
#include <map>
#include <list>
#include <boost/any.hpp>
#include <boost/iterator/filter_iterator.hpp>
#include "scope.hpp"
#include <functional>
#include <memory>
namespace AI
{
    template< typename VARIABLE_ID_T, typename VARIABLE_T >
    struct local_constraint
    {
        std::vector< VARIABLE_ID_T > related_var;
        std::function< bool( std::vector< std::reference_wrapper< const VARIABLE_T > > ) > predicate;
        local_constraint(
            const std::vector< VARIABLE_ID_T > & v,
            const std::function< bool( std::vector< std::reference_wrapper< const VARIABLE_T > > ) > & f ) :
            related_var( v ), predicate( f ) { }
        bool operator ( )( const std::map< VARIABLE_ID_T, VARIABLE_T > & partial_assignment ) const
        {
            std::vector< std::reference_wrapper< const VARIABLE_T > > arg;
            arg.reserve( related_var.size( ) );
            for ( const VARIABLE_ID_T & i : related_var )
            {
                auto it = partial_assignment.find( i );
                if ( it == partial_assignment.end( ) ) { return true; }
                arg.push_back( it->second );
            }
            return predicate( arg );
        }
    };

    template< typename VARIABLE_ID_T, typename VARIABLE_T >
    local_constraint< VARIABLE_ID_T, VARIABLE_T > make_local_constraint(
        const std::vector< VARIABLE_ID_T > & v,
        const std::function< bool( std::vector< std::reference_wrapper< const VARIABLE_T > > ) > & f )
    { return local_constraint< VARIABLE_ID_T, VARIABLE_T >( v, f ); }

    template< typename VARIABLE_ID_T, typename VARIABLE_T, typename OUTITER >
    OUTITER backtracking_search(
        std::map
        <
            VARIABLE_ID_T,
            std::pair< std::list< VARIABLE_T >, std::map< VARIABLE_ID_T, std::list< VARIABLE_T > > >
        > variable,
        std::map< VARIABLE_ID_T, VARIABLE_T > & partial_assignment,
        std::set< VARIABLE_ID_T > modify_variable,
        size_t generalized_arc_consistency_upperbound,
        std::list< local_constraint< VARIABLE_ID_T, VARIABLE_T > > & constraint_set,
        OUTITER result )
    {
        assert( partial_assignment.size( ) <= variable.size( ) );
        if ( modify_variable.size( ) == 1 )
        {
            auto mit = modify_variable.begin( );
            assert( mit != modify_variable.end( ) );
            auto pit = partial_assignment.find( * mit );
            assert( pit != partial_assignment.end( ) );
            auto vit = variable.find( * mit );
            vit->second.first = { pit->second };
        }
        if (
            std::any_of(
                constraint_set.begin( ),
                constraint_set.end( ),
                [&]( const local_constraint< VARIABLE_ID_T, VARIABLE_T > & con )
                { return ! con( partial_assignment ); } ) )
        { return result; }
        if ( partial_assignment.size( ) == variable.size( ) )
        {
            * result = partial_assignment;
            ++result;
            return result;
        }
        while ( ! modify_variable.empty( ) )
        {
            VARIABLE_ID_T current = * modify_variable.begin( );
            modify_variable.erase( current );
            std::for_each(
                constraint_set.begin( ),
                constraint_set.end( ),
                [&]( const local_constraint< VARIABLE_ID_T, VARIABLE_T > & con )
                {
                    if ( con.related_var.size( ) <= generalized_arc_consistency_upperbound &&
                        con.related_var.size( ) >= 2 &&
                        std::count( con.related_var.begin( ), con.related_var.end( ), current ) != 0 )
                    {
                        std::vector< std::reference_wrapper< std::list< VARIABLE_T > > > parameters;
                        parameters.reserve( con.related_var.size( ) );
                        std::for_each(
                            con.related_var.begin( ),
                            con.related_var.end( ),
                            [&]( const VARIABLE_ID_T & t )
                            {
                                auto it = variable.find( t );
                                assert( it != variable.end( ) );
                                parameters.push_back( it->second.first );
                            } );
                            auto shrink_domain =
                            [&]( const size_t shrink_position )
                            {
                                bool arc_prune = false;
                                for (
                                        auto it = parameters[ shrink_position ].get( ).begin( );
                                        it != parameters[ shrink_position ].get( ).end( );
                                        [&]( )
                                        {
                                            if( arc_prune == false ) { ++it; }
                                            else { arc_prune = false; }
                                        }( ) )
                                {
                                    bool need_var = false;
                                    auto generate =
                                    [&](
                                        const auto & self,
                                        std::vector< std::reference_wrapper< const VARIABLE_T > > & arg )
                                    {
                                        if ( need_var ) { return; }
                                        if ( arg.size( ) == parameters.size( ) )
                                        { if ( con.predicate( arg ) == true ) { need_var = true; } }
                                        else if ( arg.size( ) == shrink_position )
                                        {
                                            arg.push_back( * it );
                                            self( self, arg );
                                            arg.pop_back( );
                                        }
                                        else
                                        {
                                            assert( arg.size( ) != shrink_position );
                                            if ( parameters[ arg.size( ) ].get( ).empty( ) )
                                            { need_var = true; }
                                            for ( const VARIABLE_T & i : parameters[ arg.size( ) ].get( ) )
                                            {
                                                arg.push_back( i );
                                                self( self, arg );
                                                arg.pop_back( );
                                                if ( need_var ) { return; }
                                            }
                                        }
                                    };
                                std::vector< std::reference_wrapper< const VARIABLE_T > > arg;
                                generate( generate, arg );
                                if ( need_var == false )
                                {
                                    modify_variable.insert( con.related_var[ shrink_position ] );
                                    it = parameters[ shrink_position ].get( ).erase( it );
                                    arc_prune = true;
                                    auto it = variable.find( con.related_var[ shrink_position ] );
                                    assert( it != variable.end( ) );
                                    std::for_each(
                                        con.related_var.begin( ),
                                        con.related_var.end( ),
                                        [&]( const VARIABLE_ID_T & t )
                                        {
                                            if ( t != con.related_var[ shrink_position ] )
                                            {
                                                auto i = variable.find( t );
                                                assert( i != variable.end( ) );
                                                assert( ! i->second.first.empty( ) );
                                                it->second.second[ t ] = i->second.first;
                                            }
                                        } );
                                }
                        }
                    };
                    for ( size_t i = 0; i < parameters.size( ); ++i )
                    { shrink_domain( i ); }
                }
            } );
        }
        const auto & next_element =
            * std::min_element(
                variable.begin( ),
                variable.end( ),
                [&]( const auto & l, const auto & r )
                {
                    return
                        ( partial_assignment.count( l.first ) == 0 ?
                              l.second.first.size( ) : std::numeric_limits< size_t >::max( ) ) <
                        ( partial_assignment.count( r.first ) == 0 ?
                              r.second.first.size( ) : std::numeric_limits< size_t >::max( ) );
                } );
        assert( partial_assignment.count( next_element.first ) == 0 );
        if ( next_element.second.first.empty( ) )
        {
            std::vector< VARIABLE_ID_T > assigned_neighbor_ID;
            std::vector< std::list< VARIABLE_T > > assigned_neighbor_value;
            assigned_neighbor_ID.reserve( next_element.second.second.size( ) );
            assigned_neighbor_value.reserve( next_element.second.second.size( ) );
            for ( const std::pair< VARIABLE_ID_T, std::list< VARIABLE_T > > & p : next_element.second.second )
            {
                assert( next_element.first != p.first );
                assigned_neighbor_ID.push_back( p.first );
                assigned_neighbor_value.push_back( p.second );
            }
            constraint_set.push_back(
                make_local_constraint< VARIABLE_ID_T, VARIABLE_T >(
                    assigned_neighbor_ID,
                    [=]( const std::vector< std::reference_wrapper< const VARIABLE_T > > & ass )
                    {
                        assert( ass.size( ) == assigned_neighbor_value.size( ) );
                        for ( size_t i = 0; i < ass.size( ); ++i )
                        {
                            if ( std::count(
                                    assigned_neighbor_value[i].begin( ),
                                    assigned_neighbor_value[i].end( ),
                                    ass[i] ) == 0 )
                            { return true; }
                        }
                        return false;
                    } ) );
        }
        for ( const VARIABLE_T & t : next_element.second.first )
        {
            {
                auto s =
                    make_scope(
                        [&]( )
                        {
                            auto ret = partial_assignment.insert( std::make_pair( next_element.first, t ) );
                            assert( ret.second );
                        },
                        [&]( ){ partial_assignment.erase( next_element.first ); } );
                {
                    result =
                        backtracking_search(
                            variable,
                            partial_assignment,
                            { next_element.first },
                            generalized_arc_consistency_upperbound,
                            constraint_set,
                            result );
                }
                s.~scope( );
            }
            if ( std::any_of(
                    constraint_set.begin( ),
                    constraint_set.end( ),
                    [&]( const local_constraint< VARIABLE_ID_T, VARIABLE_T > & con )
                    { return ! con( partial_assignment ); } ) )
            { return result; }
        }
        return result;
    }

    template< typename VARIABLE_ID_T, typename VARIABLE_T, typename OUTITER >
    OUTITER backtracking_search(
        const std::map< VARIABLE_ID_T, std::list< VARIABLE_T > > & m,
        size_t generalized_arc_consistency_upperbound,
        std::list< local_constraint< VARIABLE_ID_T, VARIABLE_T > > constraint_set, OUTITER result )
    {
        std::map
        <
            VARIABLE_ID_T,
            std::pair< std::list< VARIABLE_T >, std::map< VARIABLE_ID_T, std::list< VARIABLE_T > > >
        > variable;
        std::set< VARIABLE_ID_T > all_element;
        std::for_each(
            m.begin( ),
            m.end( ),
            [&]( const std::pair< VARIABLE_ID_T, std::list< VARIABLE_T > > & p )
            {
                variable.insert( { p.first, { p.second, { } } } );
                all_element.insert( p.first );
            } );
        bool do_return = false;
        std::for_each(
            constraint_set.begin( ),
            constraint_set.end( ),
            [&]( const local_constraint< VARIABLE_ID_T, VARIABLE_T > & con )
            {
                if ( con.related_var.empty( ) )
                { if ( ! con.predicate( { } ) ) { do_return = true; } }
                else if ( con.related_var.size( ) == 1 )
                {
                    auto element = variable.find( con.related_var.front( ) );
                    assert( element != variable.end( ) );
                    for ( auto it = element->second.first.begin( ); it != element->second.first.end( ); ++it )
                    { if ( ! con.predicate( { * it } ) ) { it = element->second.first.erase( it ); } }
                }
            } );
        if ( do_return ) { return result; }
        std::map< VARIABLE_ID_T, VARIABLE_T > ass;
        constraint_set.remove_if(
            []( const local_constraint< VARIABLE_ID_T, VARIABLE_T > & t )
            { return t.related_var.size( ) < 2; } );
        return backtracking_search(
                variable,
                ass,
                all_element,
                generalized_arc_consistency_upperbound,
                constraint_set,
                result );
    }
}

#endif // CSP_HPP
