#pragma once

#include "ui_base.h"
#include "window.h"
#include "dialog.h"
#include "progress_bar.h"
#include "tab_set.h"
#include "combat_analizer.h"

struct set_analizer_event {
    combat_analizer*            anal;
    string_to_id_string_map*    smap;
};

class main_ui
: public ui_base {
    std::unique_ptr<dialog>                         _wnd;
    std::chrono::high_resolution_clock::time_point  _last_update;

    UINT_PTR                                _timer;

    struct combat_stat_display {
        std::unique_ptr<window>         name;
        std::unique_ptr<window>         value;
        std::unique_ptr<window>         perc;
        std::unique_ptr<progress_bar>   bar;

        combat_stat_display() {}
        combat_stat_display(combat_stat_display&& o_)
            : name(std::move(o_.name))
            , value(std::move(o_.value))
            , perc(std::move(o_.perc))
            , bar(std::move(o_.bar)) {

        }
        combat_stat_display& operator=( combat_stat_display && o_ ) {
            name = std::move(o_.name);
            value = std::move(o_.value);
            perc = std::move(o_.perc);
            bar = std::move(o_.bar);
            return *this;
        }

    private:
        combat_stat_display(const combat_stat_display&);
        combat_stat_display& operator=(const combat_stat_display&);
    };

    std::vector<combat_stat_display>        _stat_display;
    combat_analizer*                        _analizer;
    string_to_id_string_map*                _string_map;

    HWND post_param() {
        return _wnd->native_window_handle();
    }

    void update_stat_display();
    void set_analizer(const set_analizer_event& e_);
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

    LRESULT os_callback_handler(dialog* window_, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
    main_ui(const std::wstring& log_path_);
    virtual ~main_ui();
};