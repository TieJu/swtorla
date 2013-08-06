#pragma once

#include <thread>
#include <condition_variable>
#include <mutex>
#include <inplace_any.h>
#include <concurrent_queue.h>

template<typename Derived,size_t Size>
class event_queue {
    typedef tj::inplace_any<Size>   any;
    typedef Concurrency::concurrent_queue<any>  any_queue;

    any_queue           _queue;
    std::mutex          _sleep_mutex;
    std::conditional    _sleep_conditional;

protected:
    void get_events() {
        for ( ;; ) {
            any t;
            while ( !_queue.try_pop(t) ) {
                std::lock_guard<decltype( _sleep_mutex )> lock(_sleep_mutex);
                _sleep_conditional.wait(lock);
            }

            if ( !static_cast<Derived*>( this )->on_event(t) ) {
                break;
            }
        }
    }

    void peek_events() {
        any t;
        while ( _queue.try_pop(t) ) {
            if ( !static_cast<Derived*>( this )->on_event(t) ) {
                break;
            }
        }
    }

    event_queue() {}
    ~event_queue() {}

public:
    template<typename V>
    void post_event(V u_) {
        _queue.push(std::forward<V>( u_ ));
        _sleep_conditional.notify_all();
    }

    template<typename V>
    void send_event(V u_) {
        static_cast<Derived*>( this )->on_event(any(std::forward<V>( u_ )));
    }
};