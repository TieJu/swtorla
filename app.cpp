#include "app.h"

#include <sstream>
#include <future>
#include <thread>
#include <tuple>

#include <boost/property_tree/json_parser.hpp>

#include <boost/array.hpp>
#include <boost/scope_exit.hpp>


#include "to_string.h"
#include "http_state.h"

#include "path_finder.h"

#include "to_string.h"
#include "update_info_dialog.h"

#include "archive_zip.h"

#define PATCH_FILE_NAME "patch.bin"
#define WPATCH_FILE_NAME L"patch.bin"

#define CRASH_FILE_NAME "./crash.dump"


unsigned long long get_build_from_name(const std::string& name_) {
    unsigned long long c_ver = 0;
    sscanf_s(name_.c_str(), "updates/%I64u.update", &c_ver);
    return c_ver;
}

std::wstring GetStringRegKey(HKEY key_, const wchar_t* name_, const wchar_t* default_ = nullptr) {
    wchar_t szBuffer[512];
    DWORD dwBufferSize = sizeof( szBuffer );
    ULONG nError;
    nError = RegQueryValueExW(key_, name_, 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
    if ( ERROR_SUCCESS == nError ) {
        return szBuffer;
    } else if ( default_ ) {
        return default_;
    } else {
        return std::wstring();
    }
}

struct update_info {
    enum class state {
        init,
        find_server,
        load_notes,
        load_update,
        update_ok,
        update_none,
    }                                           _state = state::init;
    update_server_info                          _info;
    pplx::task<update_server_info>              _info_task;
    pplx::task<size_t>                          _download_task;
    pplx::task<std::wstring>                    _notes_task;
    app*                                        _this;
    update_dialog*                              _dlg;
    update_info_dialog*                         _info_dlg = nullptr;
};

void NTAPI app::run_update_tick( DWORD_PTR param_ ) {
    auto& _update_state = *reinterpret_cast<update_info*>( param_ );

    switch ( _update_state._state ) {
    case update_info::state::init:
        _update_state._dlg->unknown_progress( true );
        _update_state._dlg->caption( L"...updating..." );
        _update_state._dlg->info_msg( L"...downloading patch list..." );
        _update_state._info_task = _update_state._this->_updater.get_best_update_server( );
        _update_state._state = update_info::state::find_server;
        break;
    case update_info::state::find_server:
        if ( _update_state._info_task.is_done() ) {
            _update_state._info = _update_state._info_task.get();
            if ( _update_state._info.latest_version <= _update_state._this->get_program_version( ).build ) {
                _update_state._state = update_info::state::update_none;
                _update_state._dlg->destroy( );
                return;
            }
            if ( _update_state._info_dlg ) {
                _update_state._info_dlg->normal();
                _update_state._info_dlg->info_text( L"...downloading patch notes..." );
                _update_state._dlg->info_msg( L"...downloading patch notes..." );
                _update_state._notes_task = _update_state._this->_updater.download_patchnotes( _update_state._info, _update_state._this->get_program_version().build, _update_state._info.latest_version );
                _update_state._state = update_info::state::load_notes;
            } else {
                _update_state._dlg->info_msg( L"...downloading patch file..." );
                _update_state._download_task = _update_state._this->_updater.download_update( _update_state._info, _update_state._this->get_program_version().build, _update_state._info.latest_version, WPATCH_FILE_NAME );
                _update_state._state = update_info::state::load_update;
            }
        } else {
            ::Sleep( 16 );
        }
        break;
    case update_info::state::load_notes:
        if ( _update_state._notes_task.is_done() ) {
            std::wstring msg;
            try {
                msg = _update_state._notes_task.get();
            } catch ( std::exception& e_ ) {
                msg = L"...error while downloading patchnotes:\r\n" + std::to_wstring( e_.what() );
            }
            _update_state._info_dlg->info_text( msg );
            _update_state._dlg->info_msg( L"...downloading patch file..." );
            _update_state._download_task = _update_state._this->_updater.download_update( _update_state._info, _update_state._this->get_program_version( ).build, _update_state._info.latest_version, WPATCH_FILE_NAME );
            _update_state._state = update_info::state::load_update;
        } else {
            ::Sleep( 16 );
        }
        break;
    case update_info::state::load_update:
        if ( _update_state._download_task.is_done( ) ) {
            _update_state._dlg->info_msg( L"...installing update..." );
            _update_state._state = update_info::state::update_ok;
            _update_state._dlg->destroy( );
            return;
        } else {
            ::Sleep( 16 );
        }
        break;
    }

    ::QueueUserAPC( &app::run_update_tick, ::GetCurrentThread(), param_ );
}

// returns true if a new version was installed and the app needs to restart
bool app::run_update( ) {
    update_dialog dlg;
    bool do_update = true;
    bool display_info = _config.get<bool>( L"update.show_info", true );
    update_info_dialog info_dlg { L"", do_update, display_info };

    update_info info;
    info._this = this;
    info._dlg = &dlg;
    if ( display_info ) {
       info._info_dlg = &info_dlg;
    }

    ::QueueUserAPC( &app::run_update_tick, ::GetCurrentThread( ), reinterpret_cast<DWORD_PTR>( &info ) );

    dlg.peek_until( [&]() {
        return update_info::state::update_ok == info._state
            || update_info::state::update_none == info._state;
    } ); 

    if ( /*!display_info ||*/ !info_dlg.is_visiable() ) {
        info_dlg.destroy();
    }
    info_dlg.run();
    
    if ( !do_update ) {
        _config.put( L"update.auto_check", do_update );
    }

    if ( !display_info ) {
        _config.put( L"update.show_info", display_info );
    }

    if ( !display_info || !do_update ) {
        write_config( _config_path );
    }

    return update_info::state::update_ok == info._state;
}

void app::setup_from_config() {
    /*
    _config.put(L"update.auto_check", e_.cfg.check_for_updates);
    _config.put(L"update.show_info", e_.cfg.show_update_info);
    */

    auto log_level = _config.get<int>( L"app.log.level", 4 );
    auto core = boost::log::core::get();
    switch ( log_level ) {
    case 0:
        core->set_logging_enabled(false);
        break;
    case 1:
        core->set_logging_enabled(true);
        core->set_filter(boost::log::trivial::severity >= boost::log::trivial::error);
        break;
    case 2:
        core->set_logging_enabled(true);
        core->set_filter(boost::log::trivial::severity >= boost::log::trivial::warning);
        break;
    case 3:
        core->set_logging_enabled(true);
        core->set_filter(boost::log::trivial::severity >= boost::log::trivial::debug);
        break;
    case 4:
        core->set_logging_enabled(true);
        core->reset_filter();
        break;
    }
}

void app::log_entry_handler(const combat_log_entry& e_) {
    _combat_client.on_combat_event( e_ );
}

update_server_info app::check_update( update_dialog& dlg_ ) {
    dlg_.unknown_progress( true );
    auto task = _updater.get_best_update_server();
    auto server_info = task.get();
    
    if ( server_info.latest_version > _version.build ) {
        dlg_.info_msg( L"...new version on server found..." );
        BOOST_LOG_TRIVIAL( debug ) << L"...never version on server found: " << server_info.latest_version << L"...";
    } else {
        dlg_.info_msg( L"...no new version on server found..." );
        BOOST_LOG_TRIVIAL( debug ) << L"...application is up to date...";
        server_info.latest_version = 0;
    }

    return server_info;
}

void app::start_update_process(update_dialog& dlg_) {
    dlg_.info_msg(L"...applying patch...");
    BOOST_LOG_TRIVIAL(error) << L"starting image patch process";
    wchar_t file_name[MAX_PATH];
    GetModuleFileName(nullptr, file_name, MAX_PATH);

    std::wstring temp_name(file_name);
    temp_name += L".old";
    BOOST_LOG_TRIVIAL(error) << L"moving current image from " << file_name << " to " << temp_name;
    ::MoveFileExW(file_name, temp_name.c_str(), MOVEFILE_WRITE_THROUGH);

    BOOST_LOG_TRIVIAL(error) << L"moving new image from " << WPATCH_FILE_NAME << " to " << file_name;
    ::MoveFileExW(WPATCH_FILE_NAME, file_name, MOVEFILE_WRITE_THROUGH);

    STARTUPINFO si{};
    PROCESS_INFORMATION pi{};

    si.cb = sizeof( si );

    dlg_.progress(100);
    dlg_.info_msg(L"...finished, restarting...");
    BOOST_LOG_TRIVIAL(error) << L"restarting";
    CreateProcessW(file_name, L"-update", nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi);
}

void NTAPI app::remove_old_file_tick( DWORD_PTR /*param_*/ ) {
    wchar_t file_name[MAX_PATH];
    ::GetModuleFileNameW( nullptr, file_name, MAX_PATH );

    std::wstring temp_name( file_name );
    temp_name += L".old";

    BOOST_LOG_TRIVIAL( debug ) << L"removing old file " << temp_name;

    if ( !::DeleteFileW( temp_name.c_str() ) ) {
        if ( GetLastError() == ERROR_FILE_NOT_FOUND ) {
            return;
        }
    }

    ::QueueUserAPC( &app::remove_old_file_tick, ::GetCurrentThread( ), 0 );
}

void app::remove_old_file( ) {
    ::QueueUserAPC( &app::remove_old_file_tick, ::GetCurrentThread( ), 0 );
}

void app::read_config(const char* config_path_) {
    BOOST_LOG_TRIVIAL(debug) << L"reading config file from " << config_path_;
    try {
        boost::property_tree::json_parser::read_json(config_path_, _config);
    } catch ( const std::exception& e ) {
        BOOST_LOG_TRIVIAL(error) << L"unable to read config file " << config_path_ << L" because " << e.what();
    }
}

void app::write_config(const char* config_path_) {
    BOOST_LOG_TRIVIAL(debug) << L"writing config file to " << config_path_;
    try {
        boost::property_tree::json_parser::write_json(config_path_, _config);
    } catch ( const std::exception& e ) {
        BOOST_LOG_TRIVIAL(error) << L"unable to write config file " << config_path_ << L" because " << e.what();
    }

}

void NTAPI app::send_crashreport_tick( DWORD_PTR /*param_*/ ) {
    // do nothing...
}

void app::send_crashreport( ) {
    ::QueueUserAPC( &app::send_crashreport_tick, ::GetCurrentThread( ), 0 );
}

app::app(const char* caption_, const char* config_path_)
: _config_path( config_path_ )
, _main_thread( ::GetCurrentThread() ){

    INITCOMMONCONTROLSEX init =
    { sizeof( INITCOMMONCONTROLSEX ), /*ICC_PROGRESS_CLASS | ICC_TAB_CLASSES | ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES*/ 0xFFFFFFFF };
    InitCommonControlsEx( &init );

    boost::log::add_file_log(boost::log::keywords::file_name = "app.log"
                            ,boost::log::keywords::format = "[%TimeStamp%]: %Message%"
                            ,boost::log::keywords::auto_flush = true);
    boost::log::core::get()->add_global_attribute("TimeStamp", boost::log::attributes::local_clock());

    _version = find_version_info(MAKEINTRESOURCEW(VS_VERSION_INFO));

    BOOST_LOG_TRIVIAL( info ) << L"SW:ToR log analizer version " << _version.major << L"." << _version.minor << L"." << _version.patch << L" Build " << _version.build;

    send_crashreport();

    remove_old_file();

    read_config(_config_path);

    setup_from_config();

    _log_reader = log_processor { this };

    _dir_watcher = dir_watcher { this };

    _updater = updater { _config };
}


app::~app() {
    _dir_watcher.stop();
    _log_reader.stop();
    write_config(_config_path);
}


void app::operator()() {
    if ( _config.get<bool>( L"update.auto_check", true ) ) {
        if ( run_update() ) {
            return;
        }
    }

    _ui.reset(new main_ui(get_log_dir(), *this, _combat_client.get_strings(), _combat_client.get_players()));

    while ( _ui->handle_os_events() ) {
        // read combat log
        _log_reader();

        if ( _combat_client.ui_needs_update_and_reset() ) {
            _ui->update_stat_display( true );
        }
    }
}

void app::set_log_dir(const wchar_t* path_) {
    std::wstring str(path_);
    BOOST_LOG_TRIVIAL(debug) << L"log path updated to " << str;
    _config.put(L"log.path", str);
}

std::wstring app::get_log_dir() {
    auto path = _config.get<std::wstring>( L"log.path", L"" );
    if ( path.empty() ) {
        BOOST_LOG_TRIVIAL(debug) << L"swtor log path is empty, trying to find it";
        path = find_swtor_log_path();
        if ( path.empty() ) {
            BOOST_LOG_TRIVIAL(debug) << L"unable to locate swtor log path, loging ingame enabled?";
        } else {
            BOOST_LOG_TRIVIAL(debug) << L"swtor log path located at " << path;
        }
    }
    return path;
}

void app::set_program_config(const program_config& cfg_) {
    _config.put(L"update.auto_check", cfg_.check_for_updates);
    _config.put(L"update.show_info", cfg_.show_update_info);
    _config.put(L"app.log.level", cfg_.log_level);
    _config.put(L"log.path", cfg_.log_path);
    write_config(_config_path);

    setup_from_config();
}

program_config app::get_program_config() {
    program_config cfg;

    cfg.check_for_updates = _config.get<bool>( L"update.auto_check", true );
    cfg.show_update_info = _config.get<bool>( L"update.show_info", true );
    cfg.log_level = _config.get<int>( L"app.log.level", 4 );
    cfg.log_path = _config.get<std::wstring>( L"log.path", L"" );

    return cfg;
}

program_version app::get_program_version() {
    return _version;
}

bool app::start_tracking() {
    auto log_path = _config.get<std::wstring>( L"log.path", L"" );
    _dir_watcher.watch(log_path);
    auto file = find_path_for_lates_log_file(log_path);
    _log_reader.start(_current_log_file, change_log_file(file, false));
    _combat_client.new_log();
    return true;
}

void app::stop_tracking() {
    _dir_watcher.stop();
    _log_reader.stop();
    _combat_client.get_db().clear();
}

bool app::check_for_updates() {
    return run_update();
}

boost::property_tree::wptree& app::get_config() {
    return _config;
}

void app::on_listen_socket( SOCKET socket_, unsigned event_, unsigned error_ ) {
    if ( WSAENETDOWN == error_ ) {
        // notify user...
        ::closesocket( socket_ );
        return;
    }

    if ( FD_ACCEPT == event_ ) {
        SOCKET soc = INVALID_SOCKET;
        while ( INVALID_SOCKET != ( soc = ::accept( socket_, nullptr, nullptr ) ) ) {
            _combat_server.get_client_from_socket( soc );
            _ui->register_client_link_socket( soc );
        }
    }
}

void app::on_client_socket( SOCKET socket_, unsigned event_, unsigned error_ ) {
    if ( WSAENETDOWN == error_ || FD_CLOSE == event_ ) {
        _combat_client.close_socket();
    }

    if ( FD_READ == event_ ) {
        while ( _combat_client.try_recive() ) {
        }
    }
}

void app::on_any_client_socket( SOCKET socket_, unsigned event_, unsigned error_ ) {
    auto cl = _combat_server.get_client_from_socket( socket_ );

    // if the net is down, or a disconnect was requested, remove the client
    if ( WSAENETDOWN == error_ || FD_CLOSE == event_ ) {
        // notify user ...
        _combat_server.remove_client_by_socket( socket_ );
        _ui->unregister_client_link_socket( socket_ );
        // close the socket to free handle
        ::closesocket( socket_ );
        return;
    }

    if ( FD_READ == event_ ) {
        while ( cl.try_recive() ) {
        }
    }
}

void NTAPI app::on_new_log_file_change( DWORD_PTR param_ ) {
    BOOST_LOG_TRIVIAL( debug ) << L"app::on_new_log_file_change( " << param_ << L" )";
    auto self = reinterpret_cast<app*>( param_ );
    self->_log_reader.stop( );
//    self->_combat_client.get_db().clear( );
    auto time = self->change_log_file( self->_next_log_file );
    self->_log_reader.start( self->_current_log_file, time );
    self->_combat_client.new_log();
}

void app::on_new_log_file(const std::wstring& file_) {
    _next_log_file = file_;
    ::QueueUserAPC( &app::on_new_log_file_change, _main_thread, reinterpret_cast<DWORD_PTR>(this) );
}

struct tm get_date_from_file_name(const std::wstring& file_) {
    auto pos = file_.find_last_of(L'\\');

    std::wstring file_name;
    if ( pos == std::wstring::npos ) {
        file_name = file_;
    } else {
        file_name = file_.substr(pos + 1);
    }

    // format combat_<YYYY>-<MM>-<DD>_<HH>_<mm>_<ss>_<ms?>.txt
    struct tm t_info = {};
    int msec = 0;
    swscanf_s(file_name.c_str(), L"combat_%d-%d-%d_%d_%d_%d_%d.txt", &t_info.tm_year, &t_info.tm_mon, &t_info.tm_mday, &t_info.tm_hour, &t_info.tm_min, &t_info.tm_sec, &msec);

    t_info.tm_year -= 1900;
    --t_info.tm_mon;

    return t_info;
}

std::chrono::system_clock::time_point app::change_log_file( const std::wstring& file_, bool relative_ /*= true*/ ) {
    if ( relative_ ) {
        auto log_path = _config.get<std::wstring>( L"log.path", L"" );
        _current_log_file = log_path + L"\\" + file_;
    } else {
        _current_log_file = file_;
    }

    auto info = get_date_from_file_name(_current_log_file);
    // zer0 out time stuff to make it a base value for the day
    info.tm_sec = 0;
    info.tm_min = 0;
    info.tm_hour = 0;
    info.tm_wday = 0;
    info.tm_yday = 0;
    info.tm_isdst = 0;

    return std::chrono::system_clock::from_time_t( mktime( &info ) );
    
    //BOOST_LOG_TRIVIAL(debug) << L"log file " << _current_log_file << " parsed into " << info.year << "." << info.month << "." << info.day << " " << info.hour << ":" << info.minute << ":" << info.second;
}

void app::on_connected_to_server(client_net_link* self_) {
    BOOST_LOG_TRIVIAL(debug) << L"void app::on_connected_to_server(" << self_ << L");";
//    if ( _current_char < _char_list.size() ) {
        // only send name to server if we have one
//        self_->register_at_server( _char_list[_current_char] );
//    }
}

void app::on_disconnected_from_server(client_net_link* self_) {
    BOOST_LOG_TRIVIAL(debug) << L"void app::on_connected_to_server(" << self_ << L");";
    // UPDATE ui?
}

void app::on_string_lookup(client_net_link* self_, string_id string_id_) {
    BOOST_LOG_TRIVIAL(debug) << L"void app::on_string_lookup(" << self_ << L", " << string_id_ << L");";
    // server requested a string look up
//    auto ref = _string_db.find(string_id_);

//    if ( ref != end(_string_db) ) {
        // send string to server
//        self_->send_string_value(string_id_, ref->second);
//    }
}

void app::on_string_info(client_net_link * self_, string_id string_id_, const std::wstring& string_) {
    BOOST_LOG_TRIVIAL(debug) << L"void app::on_string_info(" << self_ << L", " << string_id_ << L", " << string_ << L");";
    // just insert it, if it allready exists the current value is kept
//    _string_db.insert(std::make_pair(string_id_, string_));
}

void app::on_combat_event(client_net_link * self_, const combat_log_entry& event_) {
    BOOST_LOG_TRIVIAL(debug) << L"void app::on_combat_event(" << self_ << L", " << &event_ << L");";
    // use s specialaized version of log_entry_handler to translate server name ids to local name ids
    //log_entry_handler(event_);
}

void app::on_set_name(client_net_link * self_, string_id name_id_, const std::wstring& name_) {
    BOOST_LOG_TRIVIAL(debug) << L"void app::on_set_name(" << self_ << L", " << name_id_ << L", " << name_ << L");";
    // find a free id
//    auto local = _id_map.add(name_id_);
    // ensure we have enoug slots at char list
//    _char_list.resize(max(local + 1, _char_list.size()));
    // store name
//    _char_list[local] = name_;
}

void app::new_client(c_socket socket_ ) {
    BOOST_LOG_TRIVIAL(debug) << L"void app::new_client(" << socket_.get() << L");";
//    _clients.push_back( std::make_unique<server_net_link>( this, std::move( socket_ ) ) );
}

void app::on_client_register(server_net_link * self_, const std::wstring& name_) {
    BOOST_LOG_TRIVIAL(debug) << L"void app::on_client_register(" << self_ << L", " << name_ << L");";
    //auto at = std::find(begin(_player_db), end(_player_db), name_);
    //if ( at == end(_player_db) ) {
    //    at = _player_db.insert( _player_db.end(), name_ );
    //}
    //auto id = std::distance(begin(_player_db), at);
    //for ( auto& cl : _clients ) {
    //    cl->send_set_name(id, name_);
    //}
}

void app::on_string_lookup(server_net_link* self_, string_id string_id_) {
    BOOST_LOG_TRIVIAL(debug) << L"void app::on_string_lookup(" << self_ << L", " << string_id_ << L");";
    //auto ref = _string_map.find(string_id_);
    //if ( ref == std::end(_string_map) ) {
    //    for ( auto& cl : _clients ) {
    //        cl->get_string_value(string_id_);
    //    }
    //} else {
    //    self_->send_string_value(string_id_, ref->second);
    //}
}

void app::on_string_info(server_net_link* self_, string_id string_id_, const std::wstring& string_) {
    BOOST_LOG_TRIVIAL(debug) << L"void app::on_string_lookup(" << self_ << L", " << string_id_ << L", " << string_ << L");";
    //auto at = _string_map.insert(std::make_pair(string_id_, string_)).first;

    //for ( auto& cl : _clients ) {
    //    if ( cl.get() != self_ ) {
    //        cl->send_string_value(at->first, at->second);
    //    }
    //}
}

void app::on_combat_event(server_net_link* self_, const combat_log_entry& event_) {
    BOOST_LOG_TRIVIAL(debug) << L"void app::on_combat_event(" << self_ << L", " << to_wstring(event_) << L");";
    // insert into combat table
    //_analizer.add_entry( event_ );
    //for ( auto& cl : _clients ) {
    //    if ( cl.get() != self_ ) {
    //        cl->send_combat_event(event_);
    //    }
    //}
}

void app::on_client_disconnect(server_net_link* self_) {
    BOOST_LOG_TRIVIAL(debug) << L"void app::on_client_disconnect(" << self_ << L");";
    //auto ref = find_if(begin(_clients), end(_clients), [=](const std::unique_ptr<server_net_link>& other_) { return self_ == other_.get(); });
    //if ( ref != end(_clients) ) {
    //    _clients.erase(ref);
    //}
}

void app::connect_to_server( const std::wstring& name_, const std::wstring& port_, std::function<void( unsigned error_code_ )> on_connect_ ) {
//    _client.disconnect(/* [=]( unsigned error_code_ ) {*/ );
//        _client.connect( std::to_string( name_ ), std::to_string( port_ ), on_connect_ );
    /*} );*/
}

void app::start_server( unsigned long port_ ) {
    _server_socket = c_socket { socket_api {}, AF_INET, SOCK_STREAM, 0 };
    _ui->register_listen_socket( _server_socket );

    socket_address_inet addr;
    addr.port() = htons( port_ );
    _server_socket.bind( addr );
    _server_socket.listen();
}

void app::player_change( string_id name_ ) {
    //if ( _current_char != name_ ) {
//        _current_char = name_;

//        _client.register_at_server( _char_list[name_ - 1] );

        _ui->update_main_player( name_ );
    //}
}