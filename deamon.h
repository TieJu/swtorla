#pragma once

#include "handle_wrap.h"

#include <thread>
#include <Windows.h>

#include <boost/optional.hpp>

// rename to deamon
template<typename Derived, typename State>
class deamon {
protected:
    State                   _state;
    std::thread             _thread;
    handle_wrap<HANDLE>     _server_event;
    handle_wrap<HANDLE>     _client_event;

    void run() {
        auto state = wait(State(0));
        while ( state ) {
            state = wait(*state);
        }
    }

    // blocks until _state changes to a different state then state_
    // returns the new state
    boost::optional<state> wait(state state_) {
        while ( state_ == _state ) {
            if ( ::WaitForSingleObject(*_server_event, INFINITE) == WAIT_OBJECT_0 ) {
                ::ResetEvent(*_server_event);
                break;
            }
        }

        auto do_continue = static_cast<Derived*>( this )->on_state_change(_state, state_);
        state_ = _state;

        ::SetEvent(*_client_event);

        return boost::make_optional(do_continue, state_);
    }

    // updates a state and wakes the background thread
    void change_state(state state_) {
        _state = state_;
        ::SetEvent(*_server_event);
    }

    // updates the state, wakes the background thread and waits for the background thread to pick up the change
    void change_state_and_wait(state state_) {
        _state = state_;
        ::SignalObjectAndWait(*_server_event, *_client_event, INFINITE, FALSE);
    }

    template<class Rep, class Period>
    void change_state_and_wait_for(state state_, const std::chrono::duration<Rep, Period>& rel_time_) {
        _state = state_;
        ::SignalObjectAndWait(*_server_event, *_client_event, std::chrono::duration_cast<std::chrono::milliseconds>( rel_time_ ).count(), FALSE);
    }

    deamon()
        : _state(State(0)) {}

    void start() {
        if ( !_server_event ) {
            _server_event.reset(::CreateEventW(nullptr, TRUE, FALSE, nullptr), [](HANDLE h_) { ::CloseHandle(h_); });
        }
        if ( !_client_event ) {
            _client_event.reset(::CreateEventW(nullptr, TRUE, FALSE, nullptr), [](HANDLE h_) { ::CloseHandle(h_); });
        }
        _thread = std::thread([=]() {
            run();
        });
    }

    void stop() {
        if ( _thread.joinable() ) {
            change_state_and_wait(state::shutdown);
            _thread.join();
        }
    }

    ~deamon() {
        stop();
    }

    deamon(deamon && other_)
        : deamon() {
        *this = std::move(other_);
    }

    deamon& operator=( deamon && other_ ) {
        stop();
        if ( other_._thread.joinable() ) {
            throw std::runtime_error("moved runing active object!");
        }

        _state = std::move(other_._state);
        _thread = std::move(other_._thread);
        _server_event = std::move(other_._server_event);
        _client_event = std::move(other_._client_event);
        return *this;
    }

    bool is_runging() {
        return _thread.joinable();
    }
};