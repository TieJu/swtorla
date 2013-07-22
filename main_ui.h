#pragma once

#include "ui_base.h"
#include "window.h"
#include "progress_bar.h"
#include "tab_set.h"

class main_ui : public ui_base {
    enum {
        control_path_button,
        control_path_edit,
        control_tab,
        control_start_solo_button,
        control_sync_to_raid_button,

        min_width = 600,
        min_height = 600,
    };
    window_class                            _wnd_class;
    std::unique_ptr<window>                 _wnd;
    std::unique_ptr<window>                 _path_button;
    std::unique_ptr<window>                 _path_edit;
    std::unique_ptr<tab_set>                _tab;
    std::unique_ptr<window>                 _start_solo_button;
    std::unique_ptr<window>                 _sync_raid_button;

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

    void on_size(window* window_,WPARAM wParam, LPARAM lParam);
    LRESULT os_callback_handler(window* window_, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
    main_ui(const std::wstring& log_path_);
    virtual ~main_ui();
};