#ifndef SCOPE_HPP
#define SCOPE_HPP
template< typename LEAVE >
struct scope
{
	LEAVE leave;
	template< typename ENTER >
	scope( const ENTER & enter, const LEAVE & leave ) : leave( leave ) { enter( ); }
	~scope( ) { leave( ); }
	scope( const scope & ) = delete;
	scope( scope && ) = default;
};
template< typename ENTER, typename LEAVE >
scope< LEAVE > make_scope( const ENTER & e, const LEAVE & l ) { return scope< LEAVE >( e, l ); }
#endif // SCOPE_HPP
