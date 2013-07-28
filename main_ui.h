#pragma once

#include "ui_base.h"
#include "window.h"
#include "progress_bar.h"
#include "tab_set.h"
#include "combat_analizer.h"

struct set_analizer_event {
    combat_analizer*            anal;
    string_to_id_string_map*    smap;
};

class main_ui : public ui_base {
    enum {
        control_path_button,
        control_path_edit,
        control_tab,
        control_start_solo_button,
        control_sync_to_raid_button,
        control_stop_button,

        min_width = 600,
        min_height = 600,
    };
    window_class                            _wnd_class;
    std::unique_ptr<window>                 _wnd;
    std::unique_ptr<window>                 _path_button;
    std::unique_ptr<window>                 _path_edit;
    std::unique_ptr<window>                 _start_solo_button;
    std::unique_ptr<window>                 _stop_button;
    std::unique_ptr<window>                 _dps_display;

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

    void on_size(window* window_,WPARAM wParam, LPARAM lParam);
    LRESULT os_callback_handler(window* window_, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
    main_ui(const std::wstring& log_path_);
    virtual ~main_ui();
};