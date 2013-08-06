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

class app;

class main_ui
: public ui_base {
    friend class ui_element_manager<main_ui>;

    ui_element_manager<main_ui>                     _ui_elements;
    std::unique_ptr<data_display_mode>              _data_display;
    std::unique_ptr<dialog>                         _wnd;
    std::future<bool>                               _update_state;
    UINT_PTR                                        _timer;
    combat_analizer&                                _analizer;
    string_id                                       _player_id;
    app&                                            _app;

    HWND post_param() {
        return _wnd->native_window_handle();
    }

    bool show_options_dlg();
    program_config gather_options_state(dialog* dlg_);
    INT_PTR options_dlg_handler(dialog* dlg_, UINT msg_, WPARAM w_param_, LPARAM l_param_);
    void show_about_dlg();
    INT_PTR about_dlg_handler(dialog* dlg_, UINT msg_, WPARAM w_param_, LPARAM l_param_);

    void update_stat_display();
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
protected:
    void on_start_solo();
    void on_start_raid();
    void on_stop();

public:
    main_ui(const std::wstring& log_path_, app& app_,combat_analizer& c_anal_,string_to_id_string_map& s_map_,character_list& c_list_);
    virtual ~main_ui();

    void update_main_player(string_id player_id_) {
        _player_id = player_id_;
    }
};