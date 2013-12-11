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

#include "client.h"

#include "upnp.h"
#include "local_ip.h"

#include "client_net_link.h"

#include "net_link_server.h"
#include "client_net_link.h"
#include "server_net_link.h"
#include "name_id_map.h"

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

    std::wstring scan_install_key(HKEY key_,const wchar_t* name_maptch_,bool partial_only_);
    void find_7z_path_registry();
    void find_7z_program_path_guess();
    void find_7z_start_menu();
    void find_rar_path_registry();
    void find_rar_program_path_guess();
    void find_rar_start_menue();
    void find_compress_software_start_menu();
    void find_compress_software_path_guess();
    void find_compress_software_registry();
    void find_compress_software();

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
    std::wstring get_archive_name_from_log_name(const std::wstring& name_);
    bool archive_log(const std::wstring& file_);
    void remove_log(const std::wstring& file_);

protected:
    friend class client_net_link;
    void on_connected_to_server(client_net_link* self_);
    void on_disconnected_from_server(client_net_link* self_);
    void on_string_lookup(client_net_link* self_, string_id string_id_);
    void on_string_info(client_net_link* self_, string_id string_id_, const std::wstring& string_);
    void on_combat_event(client_net_link* self_, const combat_log_entry& event_);
    void on_set_name(client_net_link* self_, string_id name_id_, const std::wstring& name_);

protected:
    friend class net_link_server;
    void new_client(c_socket socket_);

protected:
    friend class server_net_link;
    void on_client_register(server_net_link* self_, const std::wstring& name_);
    void on_client_unregister(server_net_link* self_, string_id id_);
    void on_string_lookup(server_net_link* self_, string_id string_id_);
    void on_string_info(server_net_link* self_, string_id string_id_, const std::wstring& string_);
    void on_combat_event(server_net_link* self_, const combat_log_entry& event_);
    void on_client_disconnect(server_net_link* self_);

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

    string_id map_local_to_server_id( string_id id_ ) {
        //return _id_map.map_to_server( id_ );
        return id_;
    }
    string_id map_server_to_local_id( string_id id_ ) {
        //return _id_map.map_to_local( id_ );
        return id_;
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