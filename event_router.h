#pragma once

#include <functional>
#include <concurrent_vector.h>
#include <inplace_any.h>

template<size_t MaxEventSize>
class event_router {
public:
    typedef tj::inplace_any<MaxEventSize>   any;
    typedef std::function<void (const any&)>    callback_type;

private:
    typedef concurrency::concurrent_vector<callback_type>      route_set;
    route_set   _routes;

public:
    template<typename Type,typename Clb>
    void add(Clb clb_) {
        _routes.push_back([=](const any& v_) {
            auto t = tj::any_cast<Type>( &v_ );
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