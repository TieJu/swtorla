#pragma once

#include <functional>
#include <concurrent_vector.h>

#ifdef USE_BOOST_ANY
#include <boost/any.hpp>
#define any_cast boost::any_cast
#else
#include <inplace_any.h>
#define any_cast tj::any_cast
#endif

template<size_t MaxEventSize>
class event_router {
public:
#ifdef USE_BOOST_ANY
    typedef boost::any   any;
#else
    typedef tj::inplace_any<MaxEventSize>   any;
#endif
    
    typedef std::function<void (const any&)>    callback_type;

private:
    typedef concurrency::concurrent_vector<callback_type>      route_set;
    route_set   _routes;

public:
    template<typename Type,typename Clb>
    void add(Clb clb_) {
        _routes.push_back([=](const any& v_) {
            auto t = any_cast<Type>( &v_ );
            if ( t ) {
                clb_(*t);
            }
        });
    }

    template<typename U>
    void route(U v_) {
        route(any{ std::forward<U>(v_) });
    }

    void route(const any& v_) {
        for ( auto& clb : _routes ) {
            clb(v_);
        }
    }

};