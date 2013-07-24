#pragma once

#include <mutex>
#include <concurrent_vector.h>

#define APP_EVENT ( WM_APP + 1 )


template<typename Mutex>
std::unique_lock<Mutex> lock_unique(Mutex& m_) {
    return std::unique_lock<Mutex>(m_);
}

template<typename Mutex>
std::unique_lock<Mutex> try_lock_unique(Mutex& m_) {
    return std::unique_lock<Mutex>(m_, std::try_to_lock);
}

template<typename Derived,size_t MaxEventSize>
class win32_event_queue {
public:
    typedef tj::inplace_any<MaxEventSize> any;

private:
    typedef concurrency::concurrent_vector<any>     any_buffer;
    // todo remove mutex and use atomics instead?
    std::mutex  _mutex;
    any_buffer  _event_buffer;

protected:
    template<typename Clb>
    void invoke_on(Clb clb_,size_t index_) {
        clb_(_event_buffer[index_]);
    }

    void free_slot(size_t index_) {
        _event_buffer[index_] = any{};
    }

    bool is_app_event(UINT uMsg) {
        return uMsg == APP_EVENT;
    }
    void dispatch_event(WPARAM wParam, LPARAM /*lParam*/) {
        invoke_on([=](const any& a_) { static_cast<Derived*>( this )->on_event(a_); }, wParam);
        free_slot(wParam);
    }
    void peek_events(HWND window_,UINT filter_min_ = 0,UINT filter_max_ = 0) {
        MSG msg{};
        while ( ::PeekMessageW(&msg, window_, filter_min_, filter_max_, PM_REMOVE) ) {
            if ( msg.message == APP_EVENT ) {
                dispatch_event(msg.wParam, msg.lParam);
            }
        }
    }

    void get_events(HWND window_, UINT filter_min_ = 0, UINT filter_max_ = 0) {
        MSG msg{};
        while ( ::GetMessageW(&msg, window_, filter_min_, filter_max_) ) {
            if ( msg.message == APP_EVENT ) {
                dispatch_event(msg.wParam, msg.lParam);
            }
        }
    }
    template<typename U>
    void peek_events(HWND window_, UINT filter_min_, UINT filter_max_, U v_) {
        MSG msg{};
        while ( ::PeekMessageW(&msg, window_, filter_min_, filter_max_, PM_REMOVE) ) {
            if ( msg.message == APP_EVENT ) {
                dispatch_event(msg.wParam, msg.lParam);
            }
            if ( !v_(msg) ) {
                break;
            }
        }
    }

    template<typename U>
    void get_events(HWND window_, UINT filter_min_, UINT filter_max_, U v_) {
        MSG msg{};
        while ( ::GetMessageW(&msg, window_, filter_min_, filter_max_) ) {
            if ( msg.message == APP_EVENT ) {
                dispatch_event(msg.wParam, msg.lParam);
            }
            if ( !v_(msg) ) {
                break;
            }
        }
    }

    void _post_event(HWND window_,size_t data_id_) {
        ::PostMessageW(window_, APP_EVENT, data_id_, 0);
    }
    void _post_event(DWORD thread_id_, size_t data_id_) {
        ::PostThreadMessageW(thread_id_, APP_EVENT, data_id_, 0);
    }
    void _send_event(HWND window_, size_t data_id_) {
        ::SendMessageW(window_, data_id_);
    }

protected:
    win32_event_queue() {
    }
    ~win32_event_queue() {
    }

public:
    template<typename V>
    void post_event(V u_) {
        // try to lock the mutex, if it fails, we create a new entry and dont reuse any empty space
        auto lock = try_lock_unique(_mutex);
        if ( lock ) {
            auto at = std::find_if(begin(_event_buffer), end(_event_buffer), [=](const any& a_) {
                return a_.empty();
            });
            if ( at != end(_event_buffer) ) {
                *at = std::forward<V>( u_ );
                _post_event(static_cast<Derived*>( this )->post_param(), std::distance(begin(_event_buffer), at));
                return;
            }
            lock.unlock();
        }

        auto at = _event_buffer.push_back(std::forward<V>( u_ ));
        _post_event(static_cast<Derived*>( this )->post_param(), std::distance(begin(_event_buffer), at));
    }

    template<typename V>
    void send_event(V u_) {
        // try to lock the mutex, if it fails, we create a new entry and dont reuse any empty space
        auto lock = try_lock_unique(_mutex);
        if ( lock ) {
            auto at = std::find_if(begin(_event_buffer), end(_event_buffer), [=](const any& a_) {
                return a_.empty();
            });
            if ( at != end(_event_buffer) ) {
                *at = std::move<V>( u_ );
                _send_event(static_cast<Derived*>( this )->send_param(), std::distance(begin(_event_buffer), at));
                return;
            }
            lock.unlock();
        }

        auto at = _event_buffer.push_back(std::forward<V>( u_ ));
        _send_event(static_cast<Derived*>( this )->send_param(), std::distance(begin(_event_buffer), at));
    }
};