#pragma once

#include "handle_wrap.h"

#include <thread>

#include <Windows.h>

// rename to deamon
template<typename Derived>
class active {
protected:
    enum class state {
        sleep,
        shutdown,
        run,
        init,
    };

    state                   _state;
    std::thread             _thread;
    handle_wrap<HANDLE>     _server_event;
    handle_wrap<HANDLE>     _client_event;

    // blocks until _state changes to a different state then state_
    // returns the new state
    state wait(state state_) {
        while ( state_ == _state ) {
            if ( ::WaitForSingleObject(*_server_event, INFINITE) ) {
                ::ResetEvent(*_server_event);
                break;
            }
        }

        state_ = _state;

        ::SetEvent(*_client_event);

        return state_;
    }

    // blocks until _state changes to a different state then state_ or the time runs out
    // returns the new state or state_ if the timout was hit
    template<class Rep, class Period>
    state wait_for(state state_, const std::chrono::duration<Rep, Period>& rel_time_) {
        while ( state_ == _state ) {
            auto result = ::WaitForSingleObject(*_server_event, std::chrono::duration_cast<std::chrono::milliseconds>( rel_time_ ).count());
            if ( result == WAIT_OBJECT_0 ) {
                ::ResetEvent(*_server_event);
                break;
            }
            return state_;
        }

        state_ = _state;

        ::SetEvent(*_client_event);

        return state_;
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

    active()
        : _state(state::sleep) {
    }

    void start() {
        if ( !_server_event ) {
            _server_event.reset(::CreateEventW(nullptr, TRUE, FALSE, nullptr), [](HANDLE h_) { ::CloseHandle(h_); });
        }
        if ( !_client_event ) {
            _client_event.reset(::CreateEventW(nullptr, TRUE, FALSE, nullptr), [](HANDLE h_) { ::CloseHandle(h_); });
        }
        _thread = std::thread([=]() {
            static_cast<Derived*>( this )->run();
        });
    }

    void stop() {
        if ( _thread.joinable() ) {
            change_state_and_wait(state::shutdown);
            _thread.join();
        }
    }

    ~active() {
        stop();
    }

    active(active&& other_)
        : active() {
        *this = std::move(other_);
    }

    active& operator=(active&& other_ ) {
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