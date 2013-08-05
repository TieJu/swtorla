#pragma once

#include <future>

#include "ui_base.h"
#include "window.h"
#include "dialog.h"
#include "progress_bar.h"
#include "tab_set.h"
#include "combat_analizer.h"
#include "ui_element_manager.h"
#include "data_display_mode.h"

struct set_analizer_event {
    combat_analizer*            anal;
    string_to_id_string_map*    smap;
    character_list*             names;
};

class main_ui
: public ui_base {
    ui_element_manager                              _ui_elements;
    std::unique_ptr<data_display_mode>              _data_display;
    std::unique_ptr<dialog>                         _wnd;
    std::future<bool>                               _update_state;
    UINT_PTR                                        _timer;
    combat_analizer*                                _analizer;
    string_id                                       _player_id;

    HWND post_param() {
        return _wnd->native_window_handle();
    }


    bool show_options_dlg();
    void gather_options_state(dialog* dlg_, program_config& cfg_);
    INT_PTR options_dlg_handler(dialog* dlg_, UINT msg_, WPARAM w_param_, LPARAM l_param_);
    void show_about_dlg();
    INT_PTR about_dlg_handler(dialog* dlg_, UINT msg_, WPARAM w_param_, LPARAM l_param_);

    void update_stat_display();
    void set_analizer(const set_analizer_event& e_);
    void display_log_dir_select(display_log_dir_select_event);
    virtual void on_event(const any& v_);
public:
    virtual bool handle_os_events();

private:
#if defined(USE_CUSTOME_SELECTOR)
    static void ScreenToClientX(HWND hWnd, LPRECT lpRect);
    static void MoveWindowX(HWND hWnd, RECT& rect, BOOL bRepaint);
    static void SizeBrowseDialog(HWND hWnd);
#endif
    static int CALLBACK BrowseCallbackProc(HWND hWnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
    void display_dir_select(HWND edit_);

    LRESULT os_callback_handler(dialog* window_, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
    main_ui(const std::wstring& log_path_);
    virtual ~main_ui();

    void update_main_player(string_id player_id_) {
        _player_id = player_id_;
    }
};