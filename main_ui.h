#pragma once

#include "ui_base.h"
#include "window.h"

class main_ui : public ui_base {
    window_class            _wnd_class;
    std::unique_ptr<window> _wnd;

    void display_log_dir_select(display_log_dir_select_event);
    virtual void on_event(const any& v_);
    virtual void handle_os_events();

#if defined(USE_CUSTOME_SELECTOR)
    static void ScreenToClientX(HWND hWnd, LPRECT lpRect);
    static void MoveWindowX(HWND hWnd, RECT& rect, BOOL bRepaint);
    static void SizeBrowseDialog(HWND hWnd);
#endif
    static int CALLBACK BrowseCallbackProc(HWND hWnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
    void display_dir_select();

public:
    main_ui();
    virtual ~main_ui();
};