#ifndef SEARCH_HPP
#define SEARCH_HPP
#include <stdexcept>
#include <boost/optional/optional.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <list>
#include <set>
#include <iostream>
#include <map>
#include <utility>
#include <cassert>
#include <functional>
#include <boost/function_output_iterator.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <random>
#include <tuple>
#include "../cpp_common/scope.hpp"
#include "../FunctionalCpp/combinator.hpp"
namespace AI
{
    template
    <
        typename ACTION_TYPE,
        typename STATE,
        typename COST,
        typename ALL_ACTION,
        typename NEXT_STATE,
        typename RETURN_IF,
        typename COST_OUTPUT,
        typename EVAL_FUNC,
        typename OUTITER
    >
    OUTITER best_first_search(
            const STATE & inital_state,
            const COST & inital_cost,
            ALL_ACTION action,
            NEXT_STATE next_state,
            RETURN_IF return_if,
            EVAL_FUNC eval_func,
            COST_OUTPUT cost_output,
            OUTITER result )
    {
        struct state_tag { };
        struct eval_tag { };
        using namespace boost;
        using namespace multi_index;
        typedef decltype( eval_func( std::declval< STATE >( ), std::declval< COST >( ) ) ) EVAL;
        struct element
        {
            STATE state;
            COST cost;
            std::list< STATE > history;
            EVAL eval;
            std::list< ACTION_TYPE > act;
            element(
                const STATE & state,
                const COST & cost,
                std::list< STATE > && history,
                const EVAL & eval,
                     std::list< ACTION_TYPE > && act ) :
                state( state ), cost( cost ), history( std::move( history ) ),
                eval( eval ), act( std::move( act ) ) { }
        };
        auto make_element =
                [&]( const STATE & state, const COST & cost, std::list< STATE > && history,
                     std::list< ACTION_TYPE > && act )
        { return element( state, cost, std::move( history ), eval_func( state, cost ), std::move( act ) ); };
        auto update_element = [&]( element & e ) { e.eval = eval_func( e.state, e.cost ); };
        multi_index_container
        <
            element,
            indexed_by
            <
                ordered_unique< tag< state_tag >, member< element, STATE, & element::state > >,
                ordered_non_unique< tag< eval_tag >, member< element, EVAL, & element::eval > >
            >
        > query;
        query.insert( make_element( inital_state, inital_cost, { inital_state }, { } ) );
        auto & goodness_index = query.template get< eval_tag >( );
        auto & state_index = query.template get< state_tag >( );
        while ( ! query.empty( ) )
        {
            auto iterator = goodness_index.begin( );
            const element & current_element = * iterator;
            if ( return_if( current_element.state ) )
            {
                cost_output( current_element.cost );
                std::copy( current_element.act.begin( ), current_element.act.end( ), result );
                return result;
            }
            action( current_element.state, boost::make_function_output_iterator(
                    [&]( const std::pair< ACTION_TYPE, COST > & e )
                    {
                        STATE st = next_state( current_element.state, e.first );
                        if ( std::count(
                                current_element.history.begin( ),
                                current_element.history.end( ), st ) != 0 )
                        { return; }
                        auto it = state_index.find( st );
                        COST cost = e.second + current_element.cost;
                        std::list< STATE > history = current_element.history;
                        history.push_back( st );
                        auto func = current_element.act;
                        func.push_back( e.first );
                        if ( it == state_index.end( ) )
                        {
                            state_index.insert(
                                make_element( st, cost, std::move( history ), std::move( func ) ) );
                        }
                        else { state_index.modify( it, [&]( element & ee )
                        {
                            if ( ee.cost > cost )
                            {
                                ee.cost = cost;
                                ee.history = std::move( history );
                                ee.act = std::move( func );
                                update_element( ee );
                            }
                        } ); }
                    } ) );
            goodness_index.erase( iterator );
        }
        return result;
    }

    template
    <
        typename ACTION_TYPE,
        typename STATE,
        typename COST,
        typename ALL_ACTION,
        typename NEXT_STATE,
        typename RETURN_IF,
        typename COST_OUTPUT,
        typename OUTITER
    >
    OUTITER uniform_cost_search(
            const STATE & inital_state,
            const COST & inital_cost,
            ALL_ACTION f1,
            NEXT_STATE f2,
            RETURN_IF f3,
            COST_OUTPUT f4,
            OUTITER result )
    {
        return
            best_first_search< ACTION_TYPE >(
                inital_state,
                inital_cost,
                f1,
                f2,
                f3,
                [](const STATE &, const COST & s){return s;},
                f4,
                result );
    }

    template
    <
        typename ACTION_TYPE,
        typename STATE,
        typename ALL_ACTION,
        typename NEXT_STATE,
        typename RETURN_IF,
        typename OUTITER
    >
    OUTITER breadth_first_search(
            const STATE & inital_state, ALL_ACTION f1, NEXT_STATE f2, RETURN_IF f3, OUTITER result )
    {
        size_t inital_depth = 0;
        return uniform_cost_search< ACTION_TYPE >( inital_state,
                inital_depth,
                [&]( const STATE & s, auto it )
                {
                    f1( s,
                        boost::make_function_output_iterator(
                            [&]( const ACTION_TYPE & act )
                            {
                                *it = std::make_pair(act,1);
                                ++it;
                            } ) );
                },
                f2,
                f3,
                [](size_t){},
                result );
    }

    template
    <
        typename STATE, typename COST, typename EVAL,
        typename EXPAND, typename RETURN_IF, typename COST_OUTPUT, typename SEARCH_EVAL_OUTPUT,
        typename EVAL_FUNC, typename OUTITER
    >
    OUTITER recursive_best_first_search(
            const STATE & inital_state,
            const COST & inital_cost,
            const boost::optional< EVAL > & init_eval,
            const EVAL & eval_limit,
            const std::set< STATE > & search_before,
            EXPAND f1,
            RETURN_IF f2,
            EVAL_FUNC f3,
            COST_OUTPUT f4,
            SEARCH_EVAL_OUTPUT f5,
            OUTITER result )
    {
        EVAL inital_eval( f3( inital_state, inital_cost ) );
        if ( inital_eval > eval_limit )
        {
            f5( inital_eval );
            return result;
        }
        if ( f2( inital_state ) )
        {
            f4( inital_cost );
            * result = inital_state;
            ++result;
            return result;
        }
        std::set< STATE > param = search_before;
        param.insert( inital_state );
        struct state_tag { };
        struct eval_tag { };
        using namespace boost;
        using namespace multi_index;
        struct element
        {
            STATE state;
            COST cost;
            EVAL eval;
            element( const STATE & state, const COST & cost, const EVAL & eval ) :
                state( state ), cost( cost ), eval( eval ) { }
        };
        auto make_element = [&]( const STATE & state, const COST & cost )
        {
            return element(
                state,
                cost,
                init_eval ? std::max( f3( state, cost ), * init_eval ) : f3( state, cost ) );
        };
        multi_index_container
        <
            element,
            indexed_by
            <
                ordered_unique< tag< state_tag >, member< element, STATE, & element::state > >,
                ordered_non_unique< tag< eval_tag >, member< element, EVAL, & element::eval > >
            >
        > container;
        auto & goodness_index = container.template get< eval_tag >( );
        f1(
                inital_state,
                boost::make_function_output_iterator(
                    [&]( const std::pair< STATE, COST > & p )
                    {
                        if ( search_before.count( p.first ) == 0 )
                        { container.insert( make_element( p.first, p.second + inital_cost ) ); }
                    } ) );
        if ( container.empty( ) )
        {
            f5( eval_limit * 2 );
            return result;
        }
        else if ( container.size( ) == 1 )
        {
            bool find_solution = false;
            recursive_best_first_search(
                    container.begin( )->state,
                    inital_cost + container.begin( )->cost,
                    boost::optional< EVAL >( container.begin( )->eval ),
                    eval_limit,
                    param,
                    f1,
                    f2,
                    f3,
                    f4,
                    f5,
                    boost::make_function_output_iterator(
                        std::function< void ( const STATE & ) >(
                            [&]( const STATE & s )
                            {
                                if ( ! find_solution )
                                {
                                    * result = inital_state;
                                    ++result;
                                    find_solution = true;
                                }
                                * result = s;
                                ++ result;
                            } ) ) );
            return result;
        }
        while ( true )
        {
            auto iterator = goodness_index.begin( );
            const element & current_element = * iterator;
            ++iterator;
            const element & second_element = * iterator;
            if ( current_element.eval > eval_limit )
            {
                f5( current_element.eval );
                return result;
            }
            bool find_solution = false;
            recursive_best_first_search(
                    current_element.state,
                    current_element.cost,
                    boost::optional< EVAL >( current_element.eval ),
                    std::min( eval_limit, second_element.eval ),
                    param,
                    f1,
                    f2,
                    f3,
                    f4,
                    std::function< void ( const EVAL & ) >(
                        [&](const EVAL & e)
                        {
                            goodness_index.modify(
                                goodness_index.begin( ),
                                [&](element & el){ el.eval = e; } );
                            f5( e );
                        } ),
                    boost::make_function_output_iterator(
                        std::function< void ( const STATE & ) >(
                            [&]( const STATE & s )
                            {
                                if ( ! find_solution )
                                {
                                    * result = inital_state;
                                    ++result;
                                    find_solution = true;
                                }
                                * result = s;
                                ++ result;
                            } ) ) );
            if ( find_solution ) { return result; }
        }
        return result;
    }

    template
    <
        typename STATE, typename COST, typename EVAL,
        typename EXPAND, typename RETURN_IF, typename COST_OUTPUT, typename OUTITER
    >
    OUTITER recursive_uniform_cost_search(
            const STATE & inital_state,
            const COST & inital_cost,
            const EVAL & eval_limit,
            EXPAND f1,
            RETURN_IF f2,
            COST_OUTPUT f3,
            OUTITER result )
    {
        return recursive_best_first_search(
                inital_state,
                inital_cost,
                boost::optional< EVAL >(),
                eval_limit,
                {},
                f1,
                f2,
                []( const STATE &, const COST & s ){ return s; },
                f3,
                []( const EVAL & ){},
                result );
    }

    struct postive_infinity
    {
        bool operator == ( const postive_infinity & ) { return true; }
        template< typename T > bool operator == ( const T & ) const { return false; }
        template< typename T > bool operator != ( const T & t ) const { return ! ( ( * this ) == t ); }
        template< typename T > bool operator < ( const T & ) const { return false; }
        template< typename T > bool operator >= ( const T & ) const { return true; }
        template< typename T > bool operator <= ( const T & t ) const { return ( * this ) == t; }
        template< typename T > bool operator > ( const T & t ) const { return ( * this ) != t; }
        template< typename T > postive_infinity operator + ( const T & ) const { return * this; }
        template< typename T > postive_infinity operator - ( const T & ) const { return * this; }
        postive_infinity operator - ( const postive_infinity & ) const = delete;
        template< typename T > postive_infinity operator * ( const T & t ) const { assert( t > 0 ); return * this; }
        template< typename T > postive_infinity operator / ( const T & t ) const { assert( t > 0 ); return * this; }
        template< typename T > postive_infinity & operator += ( const T & t ) { ( * this ) = ( * this ) + t; return * this; }
        template< typename T > postive_infinity & operator -= ( const T & t ) { ( * this ) = ( * this ) - t; return * this; }
        template< typename T > postive_infinity & operator *= ( const T & t ) { ( * this ) = ( * this ) * t; return * this; }
        template< typename T > postive_infinity & operator /= ( const T & t ) { ( * this ) = ( * this ) / t; return * this; }
    };

    template
    <
        typename ACTION,
        typename STATE,
        typename ALL_ACTION,
        typename NEXT_STATE,
        typename RETURN_IF,
        typename OUTITER
    >
    OUTITER depth_first_search(
            const STATE & inital_state,
            ALL_ACTION f1,
            NEXT_STATE f2,
            RETURN_IF f3,
            OUTITER result )
    { return depth_first_search< ACTION >( inital_state, postive_infinity( ), f1, f2, f3, result ); }

    template
    <
        typename ACTION,
        typename STATE,
        typename NUM,
        typename ALL_ACTION,
        typename NEXT_STATE,
        typename RETURN_IF,
        typename OUTITER
    >
    OUTITER depth_first_search(
            const STATE & inital_state,
            const NUM & depth,
            std::set< STATE > & history,
            ALL_ACTION f1,
            NEXT_STATE f2,
            RETURN_IF f3,
            OUTITER result )
    {
        if ( f3( inital_state ) ) { return result; }
        if ( depth < 1 ) { return result; }
        auto new_depth = depth - 1;
        std::vector< std::pair< STATE, ACTION > > vec;
        f1( inital_state,
            boost::make_function_output_iterator( [&](const auto & action) { vec.push_back( { f2( inital_state, action ), action } ); } ) );
        for ( const std::pair< STATE, ACTION > & s : vec )
        {
            if ( f3( s.first ) ) { * result = s.second; return ++result; }
            if ( history.count( s.first ) != 0 ) { continue; }
            history.insert( s.first );
            bool find_solution = false;
            depth_first_search< ACTION >(
                    s.first,
                    new_depth,
                    history,
                    f1,
                    f2,
                    f3,
                    boost::make_function_output_iterator(
                        std::function< void( const ACTION & ) >(
                            [&]( const ACTION & action )
                            {
                                if ( ! find_solution )
                                {
                                    * result = s.second;
                                    ++result;
                                    find_solution = true;
                                }
                                * result = action;
                                ++result;
                            } ) ) );
            if ( find_solution ) { return result; }
            history.erase( s.first );
        }
        return result;
    }

    template
    <
        typename ACTION,
        typename STATE,
        typename NUM,
        typename ALL_ACTION,
        typename NEXT_STATE,
        typename RETURN_IF,
        typename OUTITER
    >
    OUTITER depth_first_search(
            const STATE & inital_state,
            const NUM & depth,
            ALL_ACTION f1,
            NEXT_STATE f2,
            RETURN_IF f3,
            OUTITER result )
    {
        std::set< STATE > history = { inital_state };
        return depth_first_search< ACTION >( inital_state, depth, history, f1, f2, f3, result );
    }

    template
    <
        typename ACTION,
        typename STATE,
        typename ALL_ACTION,
        typename NEXT_STATE,
        typename RETURN_IF,
        typename OUTITER
    >
    OUTITER iterative_deepening_depth_first_search( const STATE & inital_state,
            ALL_ACTION f1,
            NEXT_STATE f2,
            RETURN_IF f3,
            OUTITER result )
    {
        size_t i = 0;
        bool break_loop = false;
        while ( ! break_loop )
        {
            depth_first_search< ACTION >(
                inital_state,
                i,
                f1,
                f2,
                f3,
                boost::make_function_output_iterator(
                    [&]( const ACTION & s )
                    {
                        break_loop = true;
                        * result = s;
                        ++result;
                    } ) );
            ++i;
        }
        return result;
    }


    template
    <
        typename FORWARD_ACTION,
        typename BACKWARD_ACTION,
        typename STATE,
        typename ALL_FORWARD_ACTION,
        typename ALL_BACKWARD_ACTION,
        typename NEXT_FORWARD,
        typename NEXT_BACKWARD,
        typename FORWARD_ACTION_OUTITER,
        typename BACKWARD_ACTION_OUTITER
    >
    std::pair< FORWARD_ACTION_OUTITER, BACKWARD_ACTION_OUTITER >
    biderectional_breadth_first_search(
        const STATE & inital_state,
        const STATE & final_state,
        ALL_FORWARD_ACTION all_forward_action,
        NEXT_FORWARD next_forward,
        FORWARD_ACTION_OUTITER forward_action_outiter,
        ALL_BACKWARD_ACTION all_backward_action,
        NEXT_BACKWARD next_backward,
        BACKWARD_ACTION_OUTITER backward_action_outiter )
    {
        std::list< STATE > forward { inital_state };
        std::list< STATE > backward { final_state };
        std::map< STATE, std::list< FORWARD_ACTION > >
            forward_expanded( { { inital_state, std::list< FORWARD_ACTION >( ) } } );
        std::map< STATE, std::list< BACKWARD_ACTION > >
            backward_expanded( { { final_state, std::list< BACKWARD_ACTION >( ) } } );
        bool do_forward = true;
        auto has = [&]( ) { return ! ( do_forward ? forward : backward ).empty( ); };
        auto get = [&]( ) { return do_forward ? forward.front( ) : backward.front( ); };
        auto pop = [&]( ) { (do_forward ? forward : backward).pop_front( ); };
        auto opposite_has =
            [&]( const STATE & n )
            { return ( do_forward ? backward_expanded.count( n ) : forward_expanded.count( n ) ) != 0; };
        auto output_all =
            [&]( const STATE & n )
            {
                throw std::make_pair(
                        std::copy(
                            forward_expanded[n].begin( ),
                            forward_expanded[n].end( ),
                            forward_action_outiter ),
                        std::copy(
                            backward_expanded[n].begin( ),
                            backward_expanded[n].end( ),
                            backward_action_outiter ) );
            };
        auto update =
            [&]( auto next, const auto & a, const STATE & e, auto & forward_expanded, auto & forward )
            {
                STATE n = next( e, a );
                if ( forward_expanded.count( n ) == 0 )
                {
                    auto actions = forward_expanded[e];
                    actions.push_back( a );
                    forward_expanded.insert( { n, std::move( actions ) } );
                    forward.push_back( n );
                    if ( opposite_has( n ) ) { output_all( n ); }
                }
            };
        try
        {
            while ( ( ! forward.empty( ) ) || ( ! backward.empty( ) ) )
            {
                if ( has( ) )
                {
                    STATE e = get( );
                    if ( do_forward )
                    {
                        all_forward_action(
                            e,
                            boost::make_function_output_iterator(
                                [&]( const FORWARD_ACTION & a )
                                { update( next_forward, a, e, forward_expanded, forward ); } ) );
                    }
                    else
                    {
                        all_backward_action(
                            e,
                            boost::make_function_output_iterator(
                                [&]( const BACKWARD_ACTION & a )
                                { update( next_backward, a, e, backward_expanded, backward ); } ) );
                    }
                    pop( );
                }
                do_forward = ! do_forward;
            }
            return std::make_pair( forward_action_outiter, backward_action_outiter );
        }
        catch ( std::pair< FORWARD_ACTION_OUTITER, BACKWARD_ACTION_OUTITER > e ) { return e; }
    }

    struct unit
    {
        unit operator + ( const unit & ) const { return unit( ); }
        bool operator < ( const unit & ) const { return false; }
        bool operator > ( const unit & ) const { return false; }
        bool operator <= ( const unit & ) const { return true; }
        bool operator >= ( const unit & ) const { return true; }
        bool operator != ( const unit & ) const { return false; }
        bool operator == ( const unit & ) const { return true; }
    };

    template
    <
        typename ACTION_TYPE,
        typename STATE,
        typename ALL_ACTION,
        typename EVAL_FUNC,
        typename NEXT_STATE,
        typename RETURN_IF,
        typename OUTITER
    >
    OUTITER greedy_best_first_search(
            const STATE & inital_state,
            ALL_ACTION f1,
            NEXT_STATE f2,
            EVAL_FUNC eval_func,
            RETURN_IF f3,
            OUTITER result )
    {
        return
            best_first_search< ACTION_TYPE >(
                inital_state,
                unit( ),
                [&]( const STATE & s, auto it )
                {
                    f1(
                        s,
                        boost::make_function_output_iterator(
                            [&]( const ACTION_TYPE & s ) { *it = std::make_pair( s, unit( ) ); ++it; } ) );
                },
                f2,
                f3,
                [&](const STATE & s, const unit &) { return eval_func( s ); },
                []( unit ) { },
                result );
    }

    template
    <
        typename ACTION,
        typename STATE,
        typename COST,
        typename ALL_ACTION,
        typename NEXT_STATE,
        typename EVAL,
        typename RETURN_IF,
        typename COST_OUTPUT,
        typename OUTITER
    >
    OUTITER A_star(
        const STATE & inital_state,
        const COST & inital_cost,
        ALL_ACTION f1,
        NEXT_STATE f2,
        EVAL f3,
        RETURN_IF f4,
        COST_OUTPUT f5,
        OUTITER result )
    {
        return best_first_search< ACTION >(
                inital_state,
                inital_cost,
                f1,
                f2,
                f4,
                [&](const STATE & s, const COST & c){return f3(s) + c;},
                f5,
                result );
    }

    template
    <
        typename STATE, typename COST,
        typename EXPAND, typename RETURN_IF, typename COST_OUTPUT, typename EVAL_FUNC, typename OUTITER
    >
    OUTITER memory_bounded_best_first_search(
            const STATE & inital_state,
            const COST & inital_cost,
            size_t container_limit,
            EXPAND f1,
            RETURN_IF f2,
            EVAL_FUNC f3,
            COST_OUTPUT f4,
            OUTITER result )
    {
        typedef decltype( f3( std::declval< STATE >( ), std::declval< COST >( ) ) ) EVAL;
        struct element
        {
            STATE state;
            COST cost;
            EVAL eval;
            std::list< STATE > history;
            element( const STATE & s, const COST & c, const EVAL & e, const std::list< STATE > & h ) :
                state( s ), cost( c ), eval( e ), history( h ) { }
        };
        using namespace boost;
        using namespace multi_index;
        struct state_tag { };
        struct eval_tag { };
        multi_index_container
        <
            element,
            indexed_by
            <
                ordered_unique< tag< state_tag >, member< element, STATE, & element::state > >,
                ordered_non_unique< tag< eval_tag >, member< element, EVAL, & element::eval > >
            >
        > container;
        auto & state_index = container.template get< state_tag >( );
        auto & eval_index = container.template get< eval_tag >( );
        auto add_element = [&]( const STATE & s, const COST & c, const EVAL & e, const std::list< STATE > & h )
        {
            auto it = container.insert( element( s, c, e, h ) );
            if ( ! it->first )
            {
                if ( it->second.cost > c )
                {
                    it->second.cost = c;
                    it->second.e = f3( s, c );
                    it->second.history = h;
                }
            }
        };
        auto add_parent = [&]( const element & child )
        {
            assert( ! child.history.empty( ) );
            const STATE & parent = child.history.back( );
            if ( state_index.count( parent ) == 0 )
            {
                std::list< STATE > parent_history = child.history;
                parent_history.pop_back( );
                COST reverse_cost;
                f1( child.state, boost::make_function_output_iterator(
                            [&]( const std::pair< STATE, COST > & p )
                            {
                                if ( p.first == child.state )
                                { reverse_cost = p.second; }
                            } ) );
                container.insert( element( parent, child.cost - reverse_cost, child.eval, parent_history ) );
            }
            else
            {
                state_index.modify(
                    state_index.find( parent ),
                    [&](element & e){ e.eval = std::max( e.eval, child.eval ); } );
            }
        };
        add_element( inital_state, inital_cost, f3( inital_state, inital_cost ), { } );
        while ( ! container.empty( ) )
        {
            const element & current_element = eval_index.begin( )->second;
            if ( f2( current_element.state ) )
            {
                f4( current_element.cost );
                auto res = std::copy( current_element.history.begin( ), current_element.history.end( ), result );
                *res = current_element.state;
                ++res;
                return res;
            }
            std::list< STATE > history = current_element.history;
            history.push_back( current_element.state );
            f1( current_element.state, boost::make_function_output_iterator(
                        [&]( const std::pair< STATE, COST > & p )
                        {
                            add_element(
                                p.first,
                                p.second + current_element.cost,
                                std::max( f3( p.first, p.second + current_element.cost, current_element.eval ) ),
                                history );
                        } ) );
            state_index.erase( current_element );
            while ( container.size( ) > container_limit )
            {
                const auto it = eval_index.rbegin( );
                const element & remove_element = it->second;
                if ( ! remove_element.history.empty( ) ) { add_parent( remove_element ); }
                eval_index.erase( it );
            }
        }
        return result;
    }

    template
    <
        typename STATE, typename COST, typename EVAL,
        typename EXPAND, typename RETURN_IF, typename OUTITER, typename EVAL_FUNC, typename EVAL_OUTPUT
    >
    OUTITER iterative_best_first_search_helper(
            const STATE & inital_state,
            const COST & inital_cost,
            const EVAL & upper_bound,
            std::set< STATE > & history,
            EXPAND f1,
            RETURN_IF f2,
            EVAL_FUNC f3,
            EVAL_OUTPUT f4,
            OUTITER result )
    {
        if ( f2( inital_state ) )
        {
            * result = inital_state;
            ++result;
            return result;
        }
        EVAL inital_eval = f3( inital_state, inital_cost );
        if ( upper_bound < inital_eval )
        {
            f4( inital_eval );
            return result;
        }
        std::vector< std::pair< STATE, EVAL > > vec;
        f1( inital_state, std::back_inserter( vec ) );
        for ( const std::pair< STATE, EVAL > & s : vec )
        {
            if ( history.count( s.first ) != 0 ) { continue; }
            history.insert( s.first );
            bool find_solution = false;
            iterative_best_first_search_helper(
                    s,
                    inital_cost + s.second,
                    upper_bound,
                    history,
                    f1,
                    f2,
                    f3,
                    f4,
                    boost::make_function_output_iterator(
                        std::function< void( const STATE & ) >(
                            [&]( const auto & state )
                            {
                                if ( ! find_solution )
                                {
                                    * result = inital_state;
                                    ++result;
                                    find_solution = true;
                                }
                                * result = state;
                                ++result;
                            } ) ) );
            if ( find_solution ) { return result; }
            history.erase( s );
        }
        return result;
    }

    template
    <
        typename STATE, typename COST, typename EVAL,
        typename EXPAND, typename RETURN_IF, typename OUTITER, typename EVAL_FUNC, typename EVAL_OUTPUT
    >
    OUTITER iterative_best_first_search(
            const STATE & inital_state,
            const COST & inital_cost,
            const EVAL & min_eval,
            const EVAL & upper_bound,
            EXPAND f1,
            RETURN_IF f2,
            EVAL_FUNC f3,
            OUTITER result )
    {
        EVAL e;
        std::set< STATE > history;
        while ( e < min_eval )
        {
            iterative_best_first_search_helper(
                inital_state,
                inital_cost,
                upper_bound,
                history,
                f1,
                f2,
                f3,
                [&](const EVAL & ev) { e = std::max( e, ev ); },
                result );
        }
    }

    template
    <
        typename STATE,
        typename EXPAND, typename RETURN_IF, typename OUTITER, typename EVAL_FUNC
    >
    OUTITER hill_climbing(
            const STATE & inital_state,
            EXPAND f1,
            RETURN_IF f2,
            EVAL_FUNC f3,
            OUTITER result )
    { return beam_search( inital_state, 1, f1, f2, f3, result ); }

    template
    <
        typename STATE,
        typename EXPAND, typename RETURN_IF, typename OUTITER, typename EVAL_FUNC
    >
    OUTITER beam_search(
            const STATE & inital_state,
            size_t beam_num,
            EXPAND f1,
            RETURN_IF f2,
            EVAL_FUNC f3,
            OUTITER result )
    {
        if ( beam_num < 1 ) { throw std::domain_error( "must have at least one beam" ); }
        typedef decltype( f3( inital_state ) ) EVAL;
        std::multimap< EVAL, STATE > current = { { f3( inital_state ), inital_state } };
        STATE best = inital_state;
        while ( true )
        {
            if ( f2( current ) ) { return result; }
            std::multimap< EVAL, STATE > map( current );
            for ( const auto & i : current )
            {
                f1(
                    i.second,
                    boost::make_function_output_iterator(
                        [&](const STATE & s) { map.insert( { f3(s), s } ); } ) );
            }
            while ( map.size( ) > beam_num ) { map.erase( [&](){auto tem = map.end();--tem;return tem;}() ); }
            if ( f3( best ) < f3( * map.begin( ) ) )
            {
                *result = current;
                ++result;
                best = current;
            }
            map.swap( current );
        }
        return result;
    }

    template
    <
        typename STATE, typename EP_COUNT, typename RANDOM_DEVICE,
        typename EXPAND, typename RETURN_IF, typename OUTITER, typename MOVE_PROB, typename EVAL_FUNC
    >
    OUTITER simulated_annealing(
            const STATE & inital_state,
            const EP_COUNT & max_num,
            RANDOM_DEVICE & rd,
            EXPAND f1,
            RETURN_IF f2,
            MOVE_PROB f3,
            EVAL_FUNC f4,
            OUTITER result )
    {
        EP_COUNT ep = 0;
        STATE current = inital_state;
        STATE best = inital_state;
        while ( ep < max_num )
        {
            if ( f2( current, best, ep ) ) { return result; }
            std::vector< STATE > next;
            f1(current,std::back_inserter(next));
            std::uniform_int_distribution<> uid( 0, next.size( ) - 1 );
            std::uniform_real_distribution<> urd;
            const STATE & comp_state = next[uid(rd)];
            if ( f3( current, comp_state ) > urd(rd) ) { current = comp_state; }
            if ( f4( best ) < f4( current ) )
            {
                *result = current;
                ++result;
                best = current;
            }
            ++ep;
        }
        return result;
    }

    template
    <
        typename STATE, typename STEP,
        typename DELTA, typename RETURN_IF, typename OUTITER, typename EVAL_FUNC, typename STEP_CHANGE
    >
    OUTITER gradient_descent_method(
            const STATE & inital_state,
            const STEP & step,
            DELTA f1,
            RETURN_IF f2,
            EVAL_FUNC f3,
            STEP_CHANGE f4,
            OUTITER result )
    {
        STEP a( step );
        STATE current = inital_state;
        STATE best = inital_state;
        while ( true )
        {
            if ( f2( current, best ) ) { return result; }
            current += a * f1( inital_state );
            if ( f3( best ) < f3( current ) )
            {
                *result = current;
                ++result;
                best = current;
            }
            a = f4( a );
        }
        return result;
    }

    template
    <
        typename ACTION, typename STATE,
        typename GOAL_TEST, typename ALL_ACTION, typename POSSIBLE_STATE
    >
    boost::optional< std::map< STATE, ACTION > > and_or_search(
            const STATE & inital_state,
            GOAL_TEST f1,
            ALL_ACTION f2,
            POSSIBLE_STATE f3 )
    {
        if ( f1( inital_state ) ) { return std::map< STATE, ACTION >( { } ); }
        const boost::optional< std::map< STATE, ACTION > > faliure;
        auto and_test = [&]( const auto & or_t, const std::vector< STATE > & vec, std::set< STATE > & history )
        {
            std::map< STATE, ACTION > ret;
            bool dead_end = true;
            for ( const STATE & s : vec )
            {
                if ( history.count( s ) != 0 ) { continue; }
                auto res = or_t( or_t, s, history );
                dead_end = false;
                if ( ! res ) { return faliure; }
                ret.insert( res->begin( ), res->end( ) );
            }
            return dead_end ? faliure : boost::optional< std::map< STATE, ACTION > >( ret );
        };
        auto or_test =
                [&]( const auto & self, const STATE & s, std::set< STATE > & history ) ->
                    boost::optional< std::map< STATE, ACTION > >
                {
                    if ( f1( s ) )
                    {
                        return boost::optional< std::map< STATE, ACTION > >(
                                std::map< STATE, ACTION >( { } ) );
                    }
                    boost::optional< std::map< STATE, ACTION > > ret;
                    f2(
                        s,
                        boost::make_function_output_iterator(
                        [&](const ACTION & act)
                        {
                            if ( ! ret )
                            {
                                auto g = make_scope(
                                    [&]( ){ history.insert( s ); }, [&]( ){ history.erase( s ); } );
                                std::vector< STATE > vec;
                                f3( s, act, std::back_inserter( vec ) );
                                ret = and_test( self, vec, history );
                                if ( ret ) { ret->insert( std::make_pair( s, act ) ); }
                                g.~scope( );
                            }
                        } ) );
                    return ret;
                };
        std::set< STATE > history = { };
        return or_test( or_test, inital_state, history );
    }

    template< typename ACTION, typename STATE, typename ALL_ACTION, typename MAKE_MOVE, typename IS_END, typename EVAL_FUNC >
    auto minmax_search(
            const STATE & inital_state,
            bool maximize,
            ALL_ACTION all_action,
            MAKE_MOVE make_move,
            IS_END is_end,
            EVAL_FUNC eval_func )
    {
        typedef decltype( eval_func( inital_state ) ) EVAL;
        if ( is_end( inital_state ) ) { return eval_func( inital_state ); }
        std::vector< std::pair< ACTION, EVAL > > vec;
        all_action(
            inital_state,
            boost::make_function_output_iterator(
                [&](const ACTION & act)
                { vec.push_back( std::make_pair(
                    act,
                    minmax_search< ACTION >( make_move( state, act ), ! maximize, all_action, make_move, is_end, eval_func ).second ) ); } ) );
        assert( ! vec.empty( ) );
        std::sort( vec.begin( ), vec.end( ), []( const auto & l, const auto & r ) { return l.second < r.second; } );
        return maximize ? vec.back( ) : vec.front( );
    }

    template
    <
            typename STATE,
            typename NEXT_STATE,
            typename IS_END,
            typename EVAL_FUNC,
            typename STOP_IF,
            typename OUTITER
    >
    OUTITER alpha_beta_pruning_search(
            const STATE & inital_state,
            bool maximize,
            NEXT_STATE f1,
            IS_END f2,
            EVAL_FUNC f3,
            STOP_IF f4,
            OUTITER result )
    {
        if ( f2( inital_state ) )
        {
            * result = f3( inital_state ).first;
            ++result;
            return result;
        }
        typedef decltype( f3( inital_state ).first ) EVAL;
        struct tree
        {
            const STATE & this_state;
            bool exploration_completed;
            EVAL this_eval;
            std::pair< EVAL, EVAL > bound; //First is upper bound
            bool maximize;
            std::vector< std::shared_ptr< tree > > childs;
        };
        auto explore = FC::fix(
            [&]( auto & self, const tree & t )
            {
                if ( t.exploration_completed ) { return; }
                if ( t.childs.empty( ) )
                {
                    f1( t.this_state, boost::make_function_output_iterator(
                        [&](const STATE & s)
                        {
                            auto res = f3( s );
                            t.childs.push_back(
                                std::make_shared< tree >(
                                    s,
                                    f2( s ),
                                    res.first,
                                    res.second,
                                    ! t.maximize,
                                    std::vector< std::shared_ptr< tree > >( ) ) ); } ) );
                }
                else
                {
                    assert( ! t.childs.empty( ) );
                    EVAL current_bound = std::max_element(
                            t.childs.begin( ),
                            t.childs.end( ),
                            [&]( const std::shared_ptr< tree > & l, const std::shared_ptr< tree > & r )
                            {
                                return t.maximize ? l->bound.second < l->bound.second :
                                                    r->bound.first > r->bound.first;
                            } );
                    for ( auto it = t.childs.begin( ); it != t.childs.end( ); ++it )
                    {
                        std::shared_ptr< tree > & p = * it;
                        if ( t.maximize ? p->bound.first < current_bound : p->bound.second > current_bound )
                        { it = t.childs.erase( it ); }
                        self( * p );
                    }
                }
                t.exploration_completed =
                    std::all_of(
                        t.childs.begin( ),
                        t.childs.end( ),
                        []( const std::shared_ptr< tree > & t )
                        { return t->exploration_completed; } );
                t.bound.first =
                    std::max_element(
                        t.childs.begin( ),
                        t.childs.end( ),
                        []( const std::shared_ptr< tree > & l, const std::shared_ptr< tree > & r )
                        { return l->bound.first < r->bound.first; } );
                t.bound.second =
                    std::min_element(
                        t.childs.begin( ),
                        t.childs.end( ),
                        []( const std::shared_ptr< tree > & l, const std::shared_ptr< tree > & r )
                        { return l->bound.second < r->bound.second; } );
                auto eval_comp =
                    []( const std::shared_ptr< tree > & l, const std::shared_ptr< tree > & r )
                    { return l->this_eval < r->this_eval; };
                t.this_eval =
                    t.maximize ?
                    std::max_element( t.childs.begin( ), t.hilds.end( ), eval_comp ) :
                    std::min_element( t.childs.begin( ), t.childs.end( ), eval_comp );
            } );
        auto res = f3( inital_state );
        tree root(
        {
            inital_state,
            false,
            res.first,
            res.second,
            maximize,
            std::vector< std::shared_ptr< tree > >( )
        } );
        while ( ! root.exploration_completed )
        {
            * result = root.this_eval;
            ++result;
            if ( f4( ) ) { return result; }
            explore( root );
        }
        return result;
    }
}
#endif // SEARCH_HPP
