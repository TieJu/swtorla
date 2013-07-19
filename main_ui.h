#pragma once

#include "ui_base.h"
#include "window.h"

#include <CommCtrl.h>


class main_ui : public ui_base {
    window_class            _wnd_class;
    std::unique_ptr<window> _wnd;
    std::unique_ptr<window> _progress_bar;
    std::unique_ptr<window> _status_text;



    template<typename EventType, typename HandlerType>
    bool handle_event(const any& v_, HandlerType handler_) {
        auto e = tj::any_cast<EventType>( &v_ );
        if ( e ) {
            handler_(*e);
        }
        return e != nullptr;
    }
    virtual void on_event(const any& v_) {
#define do_handle_event(type,handler) handle_event<type>(v_,[=](const type& e_) { handler(e_); })
#define do_handle_event_e(event_) do_handle_event(event_##_event,event_)
    }

    virtual void handle_os_events() {
        MSG msg
        {};
        if ( GetMessageW(&msg, _wnd->native_window_handle(), 0, 0) ) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            invoke_event_handlers(quit_event());
        }
    }

public:
    main_ui()
        : _wnd_class(L"swtorla_main_window_class") {
            _wnd_class.style(_wnd_class.style() | CS_HREDRAW | CS_VREDRAW);
            _wnd_class.register_class();
            _wnd.reset(new window(0
                , _wnd_class
                , L"SW:ToR log analizer"
                , WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE
                , CW_USEDEFAULT
                , CW_USEDEFAULT
                , 350
                , 125
                , nullptr
                , nullptr
                , _wnd_class.source_instance()));

            _progress_bar.reset(new window(0, PROGRESS_CLASS, L"", WS_CHILD | WS_VISIBLE, 50, 15, 250, 25, _wnd->native_window_handle(), nullptr, _wnd_class.source_instance()));

            _status_text.reset(new window(0, L"static", L"static", SS_CENTER | WS_CHILD | WS_VISIBLE, 50, 50, 250, 25, _wnd->native_window_handle(), nullptr, _wnd_class.source_instance()));

            _wnd->callback([=](window* window_, UINT uMsg, WPARAM wParam, LPARAM lParam) -> LRESULT {
                return os_callback_handler_default(window_, uMsg, wParam, lParam);
            });
            _wnd->update();

    }
    virtual ~main_ui() {
    }
};