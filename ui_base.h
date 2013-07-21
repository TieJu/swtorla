#pragma once

#include "window.h"

#include <vector>
#include <functional>
#include <memory>
#include <inplace_any.h>

#include "resource.h"

#include <Windows.h>

struct quit_event {};

struct set_log_dir_event {
    wchar_t*        path;
};

struct get_log_dir_event {
    std::wstring*   target;
};

struct display_log_dir_select_event {};

#define APP_EVENT (WM_APP + 1)

class ui_base {
protected:
    enum {
        event_size = sizeof( std::wstring )
    };

    typedef tj::inplace_any<event_size> any;
    typedef std::function<void ( const any& v_ )> callback_type;

private:
    std::vector<callback_type>  _callbacks;
    std::vector<any>            _eventlist;

    virtual void on_event(const any& v_) = 0;

protected:
    template<typename Event>
    void invoke_event_handlers(Event e_) {
        any evnt(e_);

        for ( auto& clb : _callbacks ) {
            clb(evnt);
        }
    }

    template<typename Event>
    void post_event(HWND target_,Event e_) {
        auto pos = store_event(e_);
        ::PostMessageW(target_, APP_EVENT, pos, 0);
    }

    template<typename Event>
    size_t store_event(Event e_) {
        auto at = std::find_if(begin(_eventlist), end(_eventlist), [=](const any& a_) {
            return a_.empty();
        });

        if ( at != end(_eventlist) ) {
            *at = std::forward<Event>( e_ );
            return size_t(std::distance(begin(_eventlist), at));
        }

        _eventlist.emplace_back(std::forward<Event>( e_ ));
        return _eventlist.size() - 1;
    }

    void dispatch_app_event(WPARAM wParam, LPARAM lParam) {
        // note: do not use refs or pointers to the _eventlist elements
        // they meight become invalid (event hander posts a new event)
        for ( auto& clb : _callbacks ) {
            clb(_eventlist[wParam]);
        }

        _eventlist[wParam] = any{};
    }

    LRESULT os_callback_handler_default(window* window_, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        if ( APP_EVENT == uMsg ) {
            this->dispatch_app_event(wParam, lParam);
            return 0;
        } else if ( uMsg == WM_CLOSE || uMsg == WM_DESTROY ) {
            ::PostQuitMessage(0);
            return 0;
        } else {
            return ::DefWindowProcW(window_->native_window_handle(), uMsg, wParam, lParam);
        }
    }

    static void setup_default_window_class(window_class& wc_) {
        wc_.style(wc_.style() | CS_HREDRAW | CS_VREDRAW);
        wc_.icon(::LoadIconW(wc_.source_instance(), MAKEINTRESOURCEW(IDI_ICON1)));
        wc_.smal_icon(::LoadIconW(wc_.source_instance(), MAKEINTRESOURCEW(IDI_ICON1)));
        wc_.mouse_cursor(::LoadCursorW(NULL, IDC_ARROW)); 
        wc_.background_brush(GetSysColorBrush(COLOR_WINDOW));
    }

    template<typename EventType, typename HandlerType>
    static bool handle_event(const any& v_, HandlerType handler_) {
        auto e = tj::any_cast<EventType>( &v_ );
        if ( e ) {
            handler_(*e);
        }
        return e != nullptr;
    }

public:
    ui_base() {}
    virtual ~ui_base() {}
    virtual void handle_os_events() = 0;

    template<typename Event>
    void send(Event e_) {
        on_event(any(e_));
    }

    template<typename EventType,typename Reciver>
    void reciver(Reciver r_) {
        _callbacks.push_back(callback_type([=](const any& v_) {
            auto v = tj::any_cast<EventType>( &v_ );
            if ( v ) {
                r_(*v);
            }
        }));
    }

    static void handle_peding_events() {
        MSG msg
        {};
        while ( GetMessage(&msg, 0, 0, 0) ) {
            if ( msg.message == WM_QUIT ) {
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
};