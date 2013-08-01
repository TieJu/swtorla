#include "update_ui.h"

#include "resource.h"

void update_ui::update_progress_info(const update_progress_info_event& e_) {
    SetWindowTextW(GetDlgItem(_wnd->native_handle(), IDC_UPDATE_INFO), e_.msg.c_str());
}

void update_ui::update_progress_error(update_progress_error_event e_) {
#ifndef PBM_SETSTATE 
#define PBM_SETSTATE            (WM_USER+16)
#define PBM_GETSTATE            (WM_USER+17)
#define PBST_NORMAL             0x0001
#define PBST_ERROR              0x0002
#define PBST_PAUSED             0x0003
#endif
    WPARAM value = 0;
    switch ( e_.level ) {
    case progress_bar::display_state::normal:
        value = PBST_NORMAL;
        break;
    case progress_bar::display_state::error:
        value = PBST_ERROR;
        break;
    case progress_bar::display_state::paused:
        value = PBST_PAUSED;
        break;
    }
    ::PostMessageW(GetDlgItem(_wnd->native_handle(), IDC_UPDATE_PROGRESS), PBM_SETSTATE, value, 0);
}

void update_ui::update_progress_waiting(update_progress_waiting_event e_) {
    auto element = GetDlgItem(_wnd->native_handle(), IDC_UPDATE_PROGRESS);
    if ( e_.waiting ) {
        ::SetWindowLongPtrW(element, GWL_STYLE, ::GetWindowLongPtrW(element, GWL_STYLE) | PBS_MARQUEE);
        ::SetWindowPos(element, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        ::PostMessageW(element, PBM_SETMARQUEE, TRUE, 0);
    } else {
        ::SetWindowLongPtrW(element, GWL_STYLE, ::GetWindowLongPtrW(element, GWL_STYLE) & ~PBS_MARQUEE);
        ::SetWindowPos(element, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }
}

void update_ui::update_progress(const update_progress_event& e_) {
    auto element = GetDlgItem(_wnd->native_handle(), IDC_UPDATE_PROGRESS);
    ::PostMessageW(element, PBM_SETRANGE32, e_.range_from, e_.range_to);
    ::PostMessageW(element, PBM_SETPOS, e_.pos, 0);
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

bool update_ui::handle_os_events() {
    MSG msg
    {};
    //while ( PeekMessage(&msg, _wnd->native_window_handle(), 0, 0, PM_REMOVE) ) {
    if ( GetMessageW(&msg, _wnd->native_window_handle(), 0, 0) ) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        return true;
    } else {
        return false;
    }
}

update_ui::update_ui() {
    _wnd.reset(new dialog(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDD_UPDATE_POPUP), nullptr));
    
    _wnd->callback([=](dialog* window_, UINT uMsg, WPARAM wParam, LPARAM lParam) -> LRESULT {
        return os_callback_handler_default(window_, uMsg, wParam, lParam);
    });
    
    _wnd->update();
}

update_ui::~update_ui() {
}