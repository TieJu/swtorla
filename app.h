#pragma once

#include <memory>
#include <future>

#include <boost/property_tree/ptree.hpp>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

#include <boost/noncopyable.hpp>

#include <WinSock2.h>
#include <Windows.h>

#include "window.h"

#include "main_ui.h"

#include "dir_watcher.h"
#include "log_processor.h"
#include "combat_analizer.h"
#include "update_dialog.h"

#include "upnp.h"
#include "local_ip.h"

#include "updater.h"

#include "socket_api.h"
#include "socket.h"

#include "combat_server.h"
#include "combat_client.h"

#include "player_db.h"
#include "string_db.h"
#include "combat_db.h"

class app : boost::noncopyable {
    const char*                                     _config_path;
    boost::property_tree::wptree                    _config;
    std::unique_ptr<main_ui>                        _ui;
    program_version                                 _version;
    dir_watcher                                     _dir_watcher;
    log_processor                                   _log_reader;
    std::wstring                                    _current_log_file;
    std::wstring                                    _next_log_file;
    updater                                         _updater;
    HANDLE                                          _main_thread;
    socket_api                                      _socket_api;
    combat_server                                   _combat_server;
    c_socket                                        _server_socket;
    combat_client                                   _combat_client;

    static void NTAPI run_update_tick( DWORD_PTR param_ );
    bool run_update();

    void setup_from_config();
protected:

    void log_entry_handler(const combat_log_entry& e_);
private:

    update_server_info check_update( update_dialog& dlg_ );
    void start_update_process(update_dialog& dlg_);
    static void NTAPI remove_old_file_tick( DWORD_PTR param_ );
    void remove_old_file();

    void read_config(const char* config_path_);
    void write_config(const char* config_path_);

    static void NTAPI send_crashreport_tick( DWORD_PTR param_ );
    void send_crashreport();

public:
    app(const char* caption_, const char* config_path_);
    ~app();

    void operator()();

protected:
    friend class main_ui;
    void set_log_dir(const wchar_t* path_);
    std::wstring get_log_dir();
    void set_program_config(const program_config& cfg_);
    program_config get_program_config();
    program_version get_program_version();
    bool start_tracking();
    void stop_tracking();
    bool check_for_updates();
    boost::property_tree::wptree& get_config();
    void on_listen_socket( SOCKET socket_, unsigned event_, unsigned error_ );
    void on_client_socket( SOCKET socket_, unsigned event_, unsigned error_ );
    void on_any_client_socket( SOCKET socket_, unsigned event_, unsigned error_ );

protected:
    friend class dir_watcher;
    static void NTAPI on_new_log_file_change( DWORD_PTR param_ );
    void on_new_log_file(const std::wstring& file_);
    std::chrono::system_clock::time_point change_log_file( const std::wstring& file_, bool relative_ = true );

protected:
    friend class raid_sync_dialog;
    void connect_to_server( const std::wstring& name_, const std::wstring& port_, std::function<void (unsigned error_code_)> on_connect_ );
    void start_server( unsigned long port_ );

    std::wstring get_current_user_name();

protected:
    friend class log_processor;

    string_db& get_string_map() { return _combat_client.get_strings(); }
    player_db& get_char_list() { return _combat_client.get_players(); }

protected:
    friend class combat_analizer;
    void player_change( string_id name_ );

public:
    template<typename Callback>
    void get_host_by_name( const std::string& name_, void* buffer_, int buffer_size_, Callback clb_ ) {
        _ui->get_host_by_name( name_, buffer_, buffer_size_, std::forward<Callback>( clb_ ) );
    }

    void register_server_link_socket( c_socket& socket_ ) {
        _ui->register_server_link_socket( socket_ );
    }

    void register_listen_socket( c_socket& socket_ ) {
        _ui->register_listen_socket( socket_ );
    }

protected:
    friend class string_db;
    void lookup_string( string_id id_ ) {
        _combat_client.send_string_query( id_ );
    }

protected:
    friend class player_db;
    void remap_player_name( string_id old_, string_id new_ ) {
        // TODO: run over all combat entrys and change them
    }

public:
    combat_client& get_client() { return _combat_client; }
};