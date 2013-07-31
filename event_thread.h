#pragma once

#include <string>
#include <thread>
#include <mutex>
#include <algorithm>
#include <concurrent_vector.h>

#include <inplace_any.h>

#include <Windows.h>

#include "win32_event_queue.h"

struct stop_thread_event {};

template<typename Derived>
class event_thread
: public win32_event_queue<event_thread<Derived>,sizeof(std::wstring)> {
    std::thread _thread;
    DWORD       _thread_id;
    bool        _run;

    friend class win32_event_queue<event_thread<Derived>, sizeof( std::wstring )>;

    void run_thread() {
        while ( _run ) {
            static_cast<Derived*>( this )->run();
        }
    }

protected:
    template<typename EventType, typename HandlerType>
    static bool do_handle_event(const any& v_, HandlerType handler_) {
        auto e = any_cast<EventType>( &v_ );
        if ( e ) {
            handler_(*e);
        }
        return e != nullptr;
    }

    DWORD post_param() {
        while ( !_thread_id ) {
            std::this_thread::yield();
        }
        return _thread_id;
    }
    
    void on_event(const any& v_) {
        static_cast<Derived*>( this )->handle_event(v_);
    }
    event_thread() : _thread_id(0) {
        _thread = std::thread([=]() {
            _thread_id = ::GetCurrentThreadId();
            _run = true;
            run_thread();
        });
    }
    ~event_thread() {
        _run = false;
        post_event(stop_thread_event{});
        _thread.join();
    }
};