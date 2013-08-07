#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>

template<typename Derived>
class active {
protected:
    enum class state {
        sleep,
        shutown,
        run,
        init,
    };

    state                   _state;
    std::thread             _thread;
    std::mutex              _mutex;
    std::condition_variable _cond;

    // blocks until _state changes to a different state then state_
    // returns the new state
    state wait(state state_) {
        std::unique_lock<std::mutex> lock(_mutex);
        while ( state_ == _state ) {
            _cond.wait(lock);
        }

        state_ = _state;

        return state_;
    }

    // blocks until _state changes to a different state then state_ or the time runs out
    // returns the new state or state_ if the timout was hit
    template<class Rep, class Period>
    state wait_for(state state_, const std::chrono::duration<Rep, Period>& rel_time_) {
        std::unique_lock<std::mutex> lock(_mutex);
        while ( state_ == _state ) {
            if ( _cond.wait_for(lock, rel_time_) == std::cv_status::timeout ) {
                return state_;
            }
        }

        state_ = _state;

        return state_;
    }

    void change_state(state state_) {
        std::unique_lock<std::mutex> lock(_mutex);
        _state = state_;
        wake();
    }

    void wake() {
        _cond.notify_one();
    }

    active() : _state(state::sleep) {
        _thread = std::thread([=]() {
            static_cast<Derived*>( this )->run();
        });
    }

    ~active() {
        change_state(state::shutown);
        _thread.join();
    }
};