#pragma once

#include "window.h"
#include "handle_wrap.h"

#include <vector>
#include <functional>
#include <memory>
#include <inplace_any.h>

#include "resource.h"

#include <Windows.h>

#include "win32_event_queue.h"

struct quit_event {};

struct set_log_dir_event {
    wchar_t*        path;
};

struct get_log_dir_event {
    std::wstring*   target;
};

struct display_log_dir_select_event {};

struct start_tracking {
    bool* ok;
};
struct stop_tracking {};

class ui_base
: public win32_event_queue<ui_base, sizeof( std::wstring )> {
public:
    typedef std::function<void ( const any& v_ )> callback_type;

private:
    std::vector<callback_type>  _callbacks;
    handle_wrap<HFONT, nullptr> _window_font;

protected:

    virtual void on_event(const any& v_) = 0;

    friend class win32_event_queue<ui_base, sizeof( std::wstring )>;
    virtual HWND post_param() = 0;

    template<typename Event>
    void invoke_event_handlers(Event e_) {
        any evnt(e_);

        for ( auto& clb : _callbacks ) {
            clb(evnt);
        }
    }

    void invoke_event_handlers(WPARAM wParam,LPARAM /*lParam*/) {
        for ( auto& clb : _callbacks ) {
            invoke_on(clb, wParam);
        }
        free_slot(wParam);
    }

    LRESULT os_callback_handler_default(window* window_, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        if ( is_app_event(uMsg) ) {
            invoke_event_handlers(wParam, lParam);
            return 0;
        } else if ( uMsg == WM_CLOSE || uMsg == WM_DESTROY ) {
            ::PostQuitMessage(0);
            return 0;
        } else {
            return ::DefWindowProcW(window_->native_window_handle(), uMsg, wParam, lParam);
        }
    }

    void load_font(const std::wstring& name) {
        _window_font.reset(::CreateFontW(0, 0, GM_ADVANCED, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, name.c_str())
                          , [](HFONT h_) { DeleteObject(h_); });
    }

    void set_font_to_window(window& wnd) {
        ::SendMessageW(wnd.native_window_handle(), WM_SETFONT, ( WPARAM )*_window_font, TRUE);
    }

    static void setup_default_window_class(window_class& wc_) {
        wc_.style(wc_.style() | CS_HREDRAW | CS_VREDRAW);
        wc_.icon(::LoadIconW(wc_.source_instance(), MAKEINTRESOURCEW(IDI_ICON1)));
        wc_.smal_icon(::LoadIconW(wc_.source_instance(), MAKEINTRESOURCEW(IDI_ICON1)));
        wc_.mouse_cursor(::LoadCursorW(NULL, IDC_ARROW)); 
        wc_.background_brush(GetSysColorBrush(CTLCOLOR_DLG));
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
    ui_base() {
        load_font(L"times new roman");
    }
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
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
};