#pragma once

#include "ui_base.h"
#include "window.h"


#include <CommCtrl.h>

struct update_progress_event {
    int     range_from;
    int     range_to;
    int     pos;
};

struct update_progress_waiting_event {
    bool    waiting;
};

enum class update_progress_error_event_level {
    normal,
    error,
};

struct update_progress_error_event {
    update_progress_error_event_level   level;
};

struct update_progress_info_event {
    std::wstring    msg;
};

class update_ui : public ui_base {
    window_class            _wnd_class;
    std::unique_ptr<window> _wnd;
    std::unique_ptr<window> _progress_bar;
    std::unique_ptr<window> _status_text;


    void update_progress_info(const update_progress_info_event& e_) {
        SetWindowText(_status_text->native_window_handle(), e_.msg.c_str());
    }

    void update_progress_error(update_progress_error_event e_) {
#ifndef PBM_SETSTATE 
#define PBM_SETSTATE (WM_USER+16)
#define PBST_NORMAL             0x0001
#define PBST_ERROR              0x0002
#define PBST_PAUSED             0x0003
#endif
        WPARAM value = 0;
        switch ( e_.level ) {
        case update_progress_error_event_level::normal:
            value = PBST_NORMAL;
            break;
        case update_progress_error_event_level::error:
            value = PBST_ERROR;
            break;
        }
        ::PostMessage(_progress_bar->native_window_handle(), PBM_SETSTATE, value, 0);
    }
    void update_progress_waiting(update_progress_waiting_event e_) {
        if ( e_.waiting ) {
            _progress_bar->style(_progress_bar->style() | PBS_MARQUEE);
            ::PostMessage(_progress_bar->native_window_handle(), PBM_SETRANGE, 0, MAKELPARAM(0, 100));
            //::PostMessage(_progress_bar->native_window_handle(), PBM_SETPOS, 0, 0);
            ::PostMessage(_progress_bar->native_window_handle(), PBM_SETMARQUEE, TRUE, 0);
        } else {
            _progress_bar->style(_progress_bar->style() & ~PBS_MARQUEE);
        }
    }
    
    void update_progress(const update_progress_event& e_) {
        ::PostMessage(_progress_bar->native_window_handle(), PBM_SETRANGE, 0, MAKELPARAM(e_.range_from, e_.range_to));
        ::PostMessage(_progress_bar->native_window_handle(), PBM_SETPOS, e_.pos, 0);
    }

    template<typename EventType,typename HandlerType>
    bool handle_event(const any& v_,HandlerType handler_) {
        auto e = tj::any_cast<EventType>( &v_ );
        if ( e ) {
            handler_(*e);
        }
        return e != nullptr;
    }
    virtual void on_event(const any& v_) {
#define do_handle_event(type,handler) handle_event<type>(v_,[=](const type& e_) { handler(e_); })
#define do_handle_event_e(event_) do_handle_event(event_##_event,event_)
        if (!( do_handle_event_e(update_progress)
            || do_handle_event_e(update_progress_waiting)
            || do_handle_event_e(update_progress_error)
            || do_handle_event_e(update_progress_info) ) ) {
            post_event(_wnd->native_window_handle(), v_);
        }
    }

    virtual void handle_os_events() {
        MSG msg
        {};
        //while ( PeekMessage(&msg, _wnd->native_window_handle(), 0, 0, PM_REMOVE) ) {
        if ( GetMessageW(&msg, _wnd->native_window_handle(), 0, 0) ) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            invoke_event_handlers(quit_event());
        }
    }

public:
    update_ui()
        : _wnd_class(L"swtorla_update_window_class") {
        setup_default_window_class(_wnd_class);
        _wnd_class.register_class();
        _wnd.reset(new window(0
                                ,_wnd_class
                                ,L"...checking for updates..."
                                , WS_MINIMIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE | WS_CAPTION | WS_SYSMENU
                                ,CW_USEDEFAULT
                                ,CW_USEDEFAULT
                                ,350
                                ,125
                                ,nullptr
                                ,nullptr
                                , _wnd_class.source_instance()));

        _progress_bar.reset(new window(0, PROGRESS_CLASS, L"", WS_CHILD | WS_VISIBLE, 50, 15, 250, 25, _wnd->native_window_handle(), nullptr, _wnd_class.source_instance()));
        update_progress_waiting_event e{ false };
        update_progress_waiting(e);

        _status_text.reset(new window(0, L"static", L"static", SS_CENTER | WS_CHILD | WS_VISIBLE, 50, 50, 250, 25, _wnd->native_window_handle(), nullptr, _wnd_class.source_instance()));

        _wnd->callback([=](window* window_, UINT uMsg, WPARAM wParam, LPARAM lParam) -> LRESULT {
            return os_callback_handler_default(window_, uMsg, wParam, lParam);
        });
        _wnd->update();

    }
    virtual ~update_ui() {
    }
};