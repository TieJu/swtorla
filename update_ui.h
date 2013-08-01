#pragma once

#include "ui_base.h"
#include "dialog.h"
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
    std::unique_ptr<dialog>         _wnd;

    HWND post_param() {
        return _wnd->native_window_handle();
    }

    void update_progress_info(const update_progress_info_event& e_);
    void update_progress_error(update_progress_error_event e_);
    void update_progress_waiting(update_progress_waiting_event e_);    
    void update_progress(const update_progress_event& e_);
    virtual void on_event(const any& v_);
    virtual bool handle_os_events();

public:
    update_ui();
    virtual ~update_ui();
};