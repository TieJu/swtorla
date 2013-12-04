#pragma once

#include <future>
#include <unordered_map>

#include "ui_base.h"
#include "window.h"
#include "dialog.h"
#include "progress_bar.h"
#include "tab_set.h"
#include "combat_analizer.h"
#include "ui_element_manager.h"
#include "data_display_mode.h"
#include "socket.h"

class app;

class main_ui
: public ui_base {
public:
    typedef std::function<void( unsigned error_code_, unsigned length_ )>    async_get_host_by_name_callback;

private:
    friend class ui_element_manager<main_ui>;

    ui_element_manager<main_ui>                                 _ui_elements;
    std::unique_ptr<data_display_mode>                          _data_display;
    std::vector<std::unique_ptr<data_display_mode>>             _data_display_history;
    std::unique_ptr<dialog>                                     _wnd;
    UINT_PTR                                                    _timer;
    combat_analizer&                                            _analizer;
    string_id                                                   _player_id;
    app&                                                        _app;
    UINT                                                        _get_host_by_name;
    std::unordered_map<HANDLE,async_get_host_by_name_callback>  _on_get_host_by_name;
    UINT                                                        _listen_socket;
    UINT                                                        _server_socket;
    UINT                                                        _any_client_socket;

    HWND post_param() {
        return _wnd->native_window_handle();
    }

    bool show_options_dlg();
    program_config gather_options_state(dialog* dlg_);
    INT_PTR options_dlg_handler(dialog* dlg_, UINT msg_, WPARAM w_param_, LPARAM l_param_);
    void show_about_dlg();
    INT_PTR about_dlg_handler(dialog* dlg_, UINT msg_, WPARAM w_param_, LPARAM l_param_);

    void update_stat_display(bool force_ = false);
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

    void set_display_mode(unsigned mode_);

public:
    main_ui(const std::wstring& log_path_, app& app_,combat_analizer& c_anal_,string_to_id_string_map& s_map_,character_list& c_list_);
    virtual ~main_ui();

    void update_main_player(string_id player_id_);
    void change_display_mode(data_display_mode* mode_);
    void change_display_mode_with_history(data_display_mode* mode_);
    void change_display_mode_reset_history(data_display_mode* mode_);
    void data_display_mode_go_history_back();
    void get_host_by_name( const std::string& name_, void* buffer_, int buffer_size_, async_get_host_by_name_callback clb_ );
    void register_server_link_socket( c_socket& socket_ );
    void register_listen_socket( c_socket& socket_ );
    void register_client_link_socket( SOCKET socket_ );
    void unregister_client_link_socket( SOCKET socket_ );
};