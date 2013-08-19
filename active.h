#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>

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
    HANDLE                  _server_event;
    HANDLE                  _client_event;

    // blocks until _state changes to a different state then state_
    // returns the new state
    state wait(state state_) {
        while ( state_ == _state ) {
            if ( ::WaitForSingleObject(_server_event, INFINITE) ) {
                ::ResetEvent(_server_event);
                break;
            }
        }

        state_ = _state;

        ::SetEvent(_client_event);

        return state_;
    }

    // blocks until _state changes to a different state then state_ or the time runs out
    // returns the new state or state_ if the timout was hit
    template<class Rep, class Period>
    state wait_for(state state_, const std::chrono::duration<Rep, Period>& rel_time_) {
        while ( state_ == _state ) {
            auto result = ::WaitForSingleObject(_server_event, std::chrono::duration_cast<std::chrono::milliseconds>( rel_time_ ).count());
            if ( result == WAIT_OBJECT_0 ) {
                ::ResetEvent(_server_event);
                break;
            }
            return state_;
        }

        state_ = _state;

        ::SetEvent(_client_event);

        return state_;
    }

    // updates a state and wakes the background thread
    void change_state(state state_) {
        _state = state_;
        ::SetEvent(_server_event);
    }
    
    void change_state_and_wait(state state_) {
        _state = state_;
        ::SignalObjectAndWait(_server_event, _client_event, INFINITE, FALSE);
    }

    template<class Rep, class Period>
    void change_state_and_wait_for(state state_, const std::chrono::duration<Rep, Period>& rel_time_) {
        _state = state_;
        ::SignalObjectAndWait(_server_event, _client_event, std::chrono::duration_cast<std::chrono::milliseconds>( rel_time_ ).count(), FALSE);
    }

    active() : _state(state::sleep) {
        _server_event = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
        _client_event = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);

        _thread = std::thread([=]() {
            static_cast<Derived*>( this )->run();
        });
    }

    ~active() {
        change_state_and_wait(state::shutdown);
        _thread.join();
        ::CloseHandle(_server_event);
        ::CloseHandle(_client_event);
    }
};