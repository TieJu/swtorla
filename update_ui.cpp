#include "update_ui.h"

void update_ui::update_progress_info(const update_progress_info_event& e_) {
    SetWindowText(_status_text->native_window_handle(), e_.msg.c_str());
}

void update_ui::update_progress_error(update_progress_error_event e_) {
    _progress_bar->state(e_.level);
}

void update_ui::update_progress_waiting(update_progress_waiting_event e_) {
    _progress_bar->marque(e_.waiting);
}

void update_ui::update_progress(const update_progress_event& e_) {
    _progress_bar->range(e_.range_from, e_.range_to);
    _progress_bar->progress(e_.pos);
}

void update_ui::on_event(const any& v_) {
#define do_handle_event(type,handler) handle_event<type>(v_,[=](const type& e_) { handler(e_); })
#define do_handle_event_e(event_) do_handle_event(event_##_event,event_)
    if ( !( do_handle_event_e(update_progress)
        || do_handle_event_e(update_progress_waiting)
        || do_handle_event_e(update_progress_error)
        || do_handle_event_e(update_progress_info) ) ) {
            post_event(v_);
    }
}

void update_ui::handle_os_events() {
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

update_ui::update_ui()
    : _wnd_class(L"swtorla_update_window_class") {
    setup_default_window_class(_wnd_class);
    _wnd_class.register_class();
    _wnd.reset( new window(0
              , _wnd_class
              , L"...checking for updates..."
              , WS_MINIMIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE | WS_CAPTION | WS_SYSMENU
              , CW_USEDEFAULT
              , CW_USEDEFAULT
              , 350
              , 125
              , nullptr
              , nullptr
              , _wnd_class.source_instance()));
    
    _progress_bar.reset(new progress_bar(50, 15, 250, 25, _wnd->native_window_handle(), 0, _wnd_class.source_instance()));

    _status_text.reset(new window(0, L"static", L"", SS_CENTER | WS_CHILD | WS_VISIBLE, 50, 50, 250, 25, _wnd->native_window_handle(), nullptr, _wnd_class.source_instance()));
    
    _wnd->callback([=](window* window_, UINT uMsg, WPARAM wParam, LPARAM lParam) -> LRESULT {
        return os_callback_handler_default(window_, uMsg, wParam, lParam);
    });
    
    _wnd->update();
}

update_ui::~update_ui() {
}