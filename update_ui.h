#pragma once

#include "ui_base.h"
#include "window.h"
#include "progress_bar.h"

#include <CommCtrl.h>

struct update_progress_event {
    int     range_from;
    int     range_to;
    int     pos;
};

struct update_progress_waiting_event {
    bool    waiting;
};

struct update_progress_info_event {
    std::wstring    msg;
};

class update_ui : public ui_base {
    window_class                    _wnd_class;
    std::unique_ptr<window>         _wnd;
    std::unique_ptr<progress_bar>   _progress_bar;
    std::unique_ptr<window>         _status_text;

    HWND post_param() {
        return _wnd->native_window_handle();
    }


    void update_progress_info(const update_progress_info_event& e_);
    void update_progress_error(update_progress_error_event e_);
    void update_progress_waiting(update_progress_waiting_event e_);    
    void update_progress(const update_progress_event& e_);
    virtual void on_event(const any& v_);
    virtual void handle_os_events();

public:
    update_ui();
    virtual ~update_ui();
};