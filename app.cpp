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

#include "updater.h"

int get_build_from_name(const std::string& name_) {
    int c_ver = 0;
    sscanf_s(name_.c_str(), "updates/%d.update", &c_ver);
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

std::wstring app::scan_install_key(HKEY key_, const wchar_t* name_match_, bool partial_only_) {
    DWORD max_length = 0;
    DWORD max_bytes = 0;
    ::RegQueryInfoKeyW(key_, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &max_length, &max_bytes, nullptr, nullptr);
    std::vector<wchar_t> name_buffer(max_length + 1);
    std::vector<BYTE> data_buffer(max_bytes);
    DWORD type;
    std::wstring name;
    bool had_dipslay_name = false;
    for ( DWORD index = 0, data_length = data_buffer.size(), name_length = name_buffer.size()
         ; ERROR_SUCCESS == ::RegEnumValueW(key_, index, name_buffer.data(), &name_length, nullptr, &type, data_buffer.data(), &data_length)
         ; ++index, data_length = data_buffer.size(), name_length = name_buffer.size() ) {
        if ( !wcscmp(L"DisplayName", name_buffer.data()) ) {
            if ( partial_only_ ) {
                if ( wcsncmp(name_match_, reinterpret_cast<const wchar_t*>( data_buffer.data() ), wcslen(name_match_)) ) {
                    return std::wstring();
                }
                had_dipslay_name = true;
            } else {
                if ( wcscmp(name_match_, reinterpret_cast<const wchar_t*>(data_buffer.data())) ) {
                    return std::wstring();
                }
                had_dipslay_name = true;
            }
        } else if ( !wcscmp(L"InstallLocation", name_buffer.data()) ) {
            name = reinterpret_cast<wchar_t*>(data_buffer.data());
        }
    }

    if ( !had_dipslay_name ) {
        return std::wstring();
    }

    return name;
}

void app::find_7z_path_registry() {
    auto target = _config.get<std::wstring>( L"external.compress.7zip.path", L"" );
    if ( !target.empty() ) {
        return;
    }
    const wchar_t* reg_path = L"SOFTWARE\\7-Zip";

    HKEY key;
    if ( ERROR_SUCCESS != ::RegOpenKeyExW(HKEY_LOCAL_MACHINE, reg_path, 0, KEY_READ, &key) ) {
        return;
    }
    BOOST_SCOPE_EXIT_ALL(= ) {
        ::RegCloseKey(key);
    };

    target = GetStringRegKey(key, L"Path");
    if ( !target.empty() ) {
        target += L"7z.exe";
        BOOST_LOG_TRIVIAL(debug) << L"Found 7zip, path to console program is: " << target;
        _config.put(L"external.compress.7zip.path", target);
    }
}

void app::find_7z_program_path_guess() {}
void app::find_7z_start_menu() {}

void app::find_rar_path_registry() {
    auto target = _config.get<std::wstring>( L"external.compress.rar.path", L"" );
    if ( !target.empty() ) {
        return;
    }
    const wchar_t* install_info = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall";

    HKEY key;
    if ( ERROR_SUCCESS != ::RegOpenKeyExW(HKEY_LOCAL_MACHINE, install_info, 0, KEY_READ, &key) ) {
        return;
    }
    BOOST_SCOPE_EXIT_ALL(= ) {
        ::RegCloseKey(key);
    };

    DWORD max_length = 0;
    ::RegQueryInfoKeyW(key, nullptr, nullptr, nullptr, nullptr, &max_length, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    std::vector<wchar_t> name_buffer(max_length + 1);
    for ( DWORD index = 0; ERROR_SUCCESS == ::RegEnumKeyW(key, index, name_buffer.data(), name_buffer.size()); ++index ) {
        std::wstring path(install_info);
        path += L"\\";
        path += name_buffer.data();

        HKEY key;
        if ( ERROR_SUCCESS != ::RegOpenKeyExW(HKEY_LOCAL_MACHINE, path.c_str(), 0, KEY_READ, &key) ) {
            continue;
        }        
        BOOST_SCOPE_EXIT_ALL(= ) {
            ::RegCloseKey(key);
        };
        target = scan_install_key(key,L"WinRAR", true);
        if ( !target.empty() ) {
            target += L"Rar.exe";
            BOOST_LOG_TRIVIAL(debug) << L"Found winrar, path to console program is: " << target;
            _config.put(L"external.compress.rar.path", target);
            break;
        }
    }
}

void app::find_rar_program_path_guess() {}
void app::find_rar_start_menue() {}

void app::find_compress_software_start_menu() {
    find_7z_program_path_guess();

    find_rar_program_path_guess();
}

void app::find_compress_software_path_guess() {
    find_7z_start_menu();

    find_rar_start_menue();
}

void app::find_compress_software_registry() {
    find_7z_path_registry();

    find_rar_path_registry();
}

void app::find_compress_software() {
    find_compress_software_registry();

    find_compress_software_start_menu();

    find_compress_software_path_guess();
/*

    auto start_programs = find_start_programs_path();
    BOOST_LOG_TRIVIAL(debug) << L"looking for files at " << start_programs;

    auto search = start_programs + L"\\*";

    WIN32_FIND_DATAW info{};
    auto handle = FindFirstFileW(search.c_str(), &info);

    if ( handle != INVALID_HANDLE_VALUE ) {
        BOOST_SCOPE_EXIT_ALL(= ) {
            FindClose(handle);
        };

        auto last_info = info;

        do {
            BOOST_LOG_TRIVIAL(debug) << L"file found " << info.cFileName;
        } while ( FindNextFileW(handle, &info) );
    }
    
    start_programs = find_start_common_programs_path();
    BOOST_LOG_TRIVIAL(debug) << L"looking for files at " << start_programs;

    search = start_programs + L"\\*";

    handle = FindFirstFileW(search.c_str(), &info);

    if ( handle != INVALID_HANDLE_VALUE ) {
        BOOST_SCOPE_EXIT_ALL(= ) {
            FindClose(handle);
        };

        auto last_info = info;

        do {
            BOOST_LOG_TRIVIAL(debug) << L"file found " << info.cFileName;
        } while ( FindNextFileW(handle, &info) );
    }*/
}

std::string app::load_update_info(const std::string& name_) {
    auto remote_server = std::to_string(_config.get<std::wstring>( L"update.server", L"homepages.thm.de" ));
    auto remote_path = std::to_string(_config.get<std::wstring>( L"update.path", L"/~hg14866/swtorla/info.php?" ));
    auto command = "from=" + std::to_string(_version.build) + "&to=" + std::to_string(get_build_from_name(name_));
    auto request = std::string("GET ") + remote_path + command + " HTTP/1.1\r\n"
        "Host: " + remote_server + "\r\n"
        "Connection: close\r\n"
        "\r\n\r\n";
    std::string response;

    BOOST_LOG_TRIVIAL(debug) << L"looking up remote server address for " << remote_server;
    boost::asio::ip::tcp::resolver tcp_lookup(_io_service);
    boost::asio::ip::tcp::resolver::query remote_query(remote_server, "http");
    auto list = tcp_lookup.resolve(remote_query);

    BOOST_LOG_TRIVIAL(debug) << L"connecting to remote server";
    boost::asio::ip::tcp::socket socket(_io_service);
    boost::system::error_code error;
    boost::asio::ip::tcp::resolver::iterator lend;
    for ( ; lend != list; ++list ) {
        if ( !socket.connect(*list, error) ) {
            BOOST_LOG_TRIVIAL(debug) << L"connected to " << list->endpoint();
            break;
        }
    }

    if ( error ) {
        BOOST_LOG_TRIVIAL(error) << L"unable to connect to remote server";
        throw boost::system::system_error(error);
    }

    BOOST_LOG_TRIVIAL(debug) << L"sending request to server " << request;
    if ( !socket.write_some(boost::asio::buffer(request), error) ) {
        BOOST_LOG_TRIVIAL(error) << L"failed to send request to server";
        throw boost::system::system_error(error);
    }

    boost::array<char, 1024> buf;
    http_state<content_store_stream<std::stringstream>> state;
    bool header_written = false;

    while ( state ) {
        auto len = socket.read_some(boost::asio::buffer(buf), error);

        if ( error == boost::asio::error::eof ) {
            break;
        } else if ( error ) {
            throw boost::system::system_error(error);
        }

        state(buf.begin(), buf.begin() + len);

        if ( state.http_result() != 200 ) {
            throw std::runtime_error(std::string("http request error, expected code 200, but got ") + std::to_string(state.http_result()));
        }

        if ( state.is_header_finished() && !header_written ) {
            header_written = true;
            BOOST_LOG_TRIVIAL(debug) << L"recived response from server";
            BOOST_LOG_TRIVIAL(debug) << L"http header";
            BOOST_LOG_TRIVIAL(debug) << L"http status code " << state.http_result();
            for ( auto& kvp : state.header() ) {
                BOOST_LOG_TRIVIAL(debug) << kvp.first << ": " << kvp.second;
            }
        }
    }


    BOOST_LOG_TRIVIAL(debug) << L"closing connection to server";
    socket.close();
    BOOST_LOG_TRIVIAL(debug) << L"result was: " << state.content().stream.str();

    return state.content().stream.str();
}

std::future<void> app::show_update_info(const std::string& name_) {
    return std::async(std::launch::async, [=]() {
        if ( !_config.get<bool>( L"update.show_info", true ) ) {
            return;
        }

        std::string info = "";
        
        try {
            info = load_update_info(name_);
        } catch ( const std::exception& e_ ) {
            info = "Error while loading update informations: ";
            info += e_.what();
        }

        if ( info.empty() ) {
            info = "No update information available";
        }

        bool do_update = true;
        bool display_info = true;
        update_info_dialog dlg(info,&do_update,&display_info);
        MSG msg{};
        while ( GetMessageW(&msg, nullptr, 0, 0) ) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        if ( !do_update ) {
            _config.put( L"update.auto_check", false );
        }

        if ( !display_info ) {
            _config.put(L"update.show_info", false);
        }

        if ( !display_info || !do_update ) {
            write_config(_config_path);
        }
    });
}

bool app::run_update_async_job(update_dialog& dlg_) {
    auto str = check_update(dlg_);

    if ( str.empty() ) {
        return false;
    }

    auto display = show_update_info(str);

    str = download_update(dlg_,str);

    if ( str.empty() ) {
        return false;
    }

    start_update_process(dlg_);

    try {
        display.get();
    } catch ( const std::exception& e_ ) {
        BOOST_LOG_TRIVIAL(error) << L"update info diplay failed, because: " << e_.what();
    }

    return true;
}

// returns true if a new version was installed and the app needs to restart
bool app::run_update_async() {
    update_dialog dlg;

    auto job = std::async(std::launch::async, [=, &dlg]() { return run_update_async_job(dlg); });

    bool result = false;

    MSG msg{};
    while ( job.wait_for(std::chrono::milliseconds(100)) != std::future_status::ready ) {
        while ( ::PeekMessageW( &msg, dlg.native_handle(), 0, 0, PM_REMOVE ) ) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    dlg.destroy();

    try {
        result = job.get();
    } catch ( const std::exception& e_ ) {
        BOOST_LOG_TRIVIAL(error) << L"run_async_job failed because: " << e_.what();
    }

    while ( ::GetMessageW( &msg, dlg.native_handle(), 0, 0 ) ) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return result;
}

std::future<bool> app::run_update() {
    return std::async(std::launch::async, [=]() { return run_update_async(); });
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
    _analizer.add_entry(e_);

    // TODO: let the analizer call into app with an info about changed char?
    if ( e_.effect_action == ssc_Event && e_.effect_type == ssc_EnterCombat ) {
        // update current char, tell the server and the ui that the char has changed
        if ( _current_char != e_.src ) {
            _current_char = e_.src;

            _client.register_at_server(_char_list[e_.src]);

            _ui->update_main_player(e_.src);
        }
    }
#if 0
    auto packed = compress(e_);
    auto& buf = std::get<0>( packed );
    auto unpacked = uncompress(buf.data(), 0);
    BOOST_LOG_TRIVIAL(debug) << L"compressed log entry from "
                             << ( sizeof( e_ ) * 8 )
                             << " bits ( "
                             << sizeof( e_ )
                             << L" ) down to "
                             << std::get<1>( packed )
                             << L" bits ( "
                             << ((std::get<1>(packed) + 7) / 8)
                             << L")";
    /*if ( memcmp(&std::get<0>( unpacked ), &e_, sizeof( e_ )) ) {
        BOOST_LOG_TRIVIAL(debug) << L"compress / decompress error!";
    }*/
    auto a = std::get<0>( unpacked );
    auto a_ptr = reinterpret_cast<const unsigned char*>( &a );
    auto b_ptr = reinterpret_cast<const unsigned char*>( &e_ );
    for ( size_t i = 0; i < sizeof( e_ ); ++i ) {
        if ( a_ptr[i] != b_ptr[i] ) {
            BOOST_LOG_TRIVIAL( debug ) << L"compress / decompress error at byte " << i;
        }
    }
#endif
    //
    //// compress test
    //char buffer[sizeof(e_)];
    //auto from = std::begin(buffer);
    //auto to = std::end(buffer);

    //auto get_length_of = [=](char* res_) {
    //    return std::distance(from, res_);
    //};

    //long long length = 0;
    //length += get_length_of(pack_int(from, to, e_.time_index.hours));
    //length += get_length_of(pack_int(from, to, e_.time_index.minutes));
    //length += get_length_of(pack_int(from, to, e_.time_index.seconds));
    //length += get_length_of(pack_int(from, to, e_.time_index.milseconds));

    //length += get_length_of(pack_int(from, to, e_.src));
    //length += get_length_of(pack_int(from, to, e_.src_minion));
    //length += get_length_of(pack_int(from, to, e_.src_id));

    //length += get_length_of(pack_int(from, to, e_.dst));
    //length += get_length_of(pack_int(from, to, e_.dst_minion));
    //length += get_length_of(pack_int(from, to, e_.dst_id));

    //length += get_length_of(pack_int(from, to, e_.ability));

    //length += get_length_of(pack_int(from, to, e_.effect_action));
    //length += get_length_of(pack_int(from, to, e_.effect_type));
    //length += get_length_of(pack_int(from, to, e_.effect_value));
    //length += get_length_of(pack_int(from, to, e_.was_crit_effect));
    //length += get_length_of(pack_int(from, to, e_.effect_value_type));
    //length += get_length_of(pack_int(from, to, e_.effect_value2));
    //length += get_length_of(pack_int(from, to, e_.was_crit_effect2));
    //length += get_length_of(pack_int(from, to, e_.effect_value_type2));
    //length += get_length_of(pack_int(from, to, e_.effect_thread));

    //unsigned bit_length = 0;
    //bit_length = ( bit_pack_int(from, bit_length, e_.time_index.hours) );
    //bit_length = ( bit_pack_int(from, bit_length, e_.time_index.minutes) );
    //bit_length = ( bit_pack_int(from, bit_length, e_.time_index.seconds) );
    //bit_length = ( bit_pack_int(from, bit_length, e_.time_index.milseconds) );

    //bit_length = ( bit_pack_int(from, bit_length, e_.src) );
    //bit_length = ( bit_pack_int(from, bit_length, e_.src_minion) );
    //bit_length = ( bit_pack_int(from, bit_length, e_.src_id) );

    //bit_length = ( bit_pack_int(from, bit_length, e_.dst) );
    //bit_length = ( bit_pack_int(from, bit_length, e_.dst_minion) );
    //bit_length = ( bit_pack_int(from, bit_length, e_.dst_id) );

    //bit_length = ( bit_pack_int(from, bit_length, e_.ability) );

    //bit_length = ( bit_pack_int(from, bit_length, e_.effect_action) );
    //bit_length = ( bit_pack_int(from, bit_length, e_.effect_type) );
    //bit_length = ( bit_pack_int(from, bit_length, e_.effect_value) );
    //bit_length = ( bit_pack_int(from, bit_length, e_.was_crit_effect) );
    //bit_length = ( bit_pack_int(from, bit_length, e_.effect_value_type) );
    //bit_length = ( bit_pack_int(from, bit_length, e_.effect_value2) );
    //bit_length = ( bit_pack_int(from, bit_length, e_.was_crit_effect2) );
    //bit_length = ( bit_pack_int(from, bit_length, e_.effect_value_type2) );
    //bit_length = ( bit_pack_int(from, bit_length, e_.effect_thread) );

    //BOOST_LOG_TRIVIAL(debug) << L"bit packing " << ( bit_length / 8 ) << " vs byte packing " << length << " vs uncompressed " << sizeof( e_ );
}

std::string app::check_update(update_dialog& dlg_) {
    updater u( _config );
    auto task = u.query_build(dlg_);
    auto max_ver = task.get();
    auto new_version_path = "update/" + std::to_string( max_ver ) + ".update";
    
    if ( max_ver > _version.build ) {
        dlg_.info_msg( L"...new version on server found..." );
        BOOST_LOG_TRIVIAL( debug ) << L"...never version on server found: " << new_version_path << L"...";
    } else {
        dlg_.info_msg( L"...no new version on server found..." );
        BOOST_LOG_TRIVIAL( debug ) << L"...application is up to date...";
        new_version_path.clear();
    }

    dlg_.progress( 50 );

    return new_version_path;
#if 0

    dlg_.progress(0);
    dlg_.info_msg(L"...locating patch server...");
    auto remote_server = std::to_string(_config.get<std::wstring>( L"update.server", L"homepages.thm.de" ));
    auto remote_path = std::to_string(_config.get<std::wstring>( L"update.command", L"/~hg14866/swtorla/update.php?a=" ));
    auto target_arch = std::to_string(_config.get<std::wstring>( L"app.arch",
#ifdef _M_X64
        L"x64"
#else
        L"x86"
#endif
        ));
    auto request = std::string("GET ") + remote_path + target_arch + " HTTP/1.1\r\n"
                   "Host: " + remote_server + "\r\n"
                   "Connection: close\r\n"
                   "\r\n\r\n";
    std::string new_version_path;

    dlg_.progress(10);
    dlg_.info_msg(L"...locating patch server...");
    BOOST_LOG_TRIVIAL(debug) << L"looking up remote server address for " << remote_server;
    boost::asio::ip::tcp::resolver tcp_lookup(_io_service);
    boost::asio::ip::tcp::resolver::query remote_query(remote_server, "http");
    auto list = tcp_lookup.resolve(remote_query);

    dlg_.progress(20);
    dlg_.info_msg(L"...connecting to patch server...");
    BOOST_LOG_TRIVIAL(debug) << L"connecting to remote server";
    boost::asio::ip::tcp::socket socket(_io_service);
    boost::system::error_code error;
    boost::asio::ip::tcp::resolver::iterator lend;
    for ( ; lend != list; ++list ) {
        if ( !socket.connect(*list, error) ) {
            BOOST_LOG_TRIVIAL(debug) << L"connected to " << list->endpoint();
            break;
        }
    }

    if ( error ) {
        dlg_.info_msg(L"...unable to connect to patch server...");
        BOOST_LOG_TRIVIAL(error) << L"unable to connect to remote server";
        throw boost::system::system_error(error);
    }

    dlg_.info_msg(L"...requesting patch data...");
    BOOST_LOG_TRIVIAL(debug) << L"sending request to server " << request;
    if ( !socket.write_some(boost::asio::buffer(request), error) ) {
        BOOST_LOG_TRIVIAL(error) << L"failed to send request to server";
        throw boost::system::system_error(error);
    }

    dlg_.unknown_progress(true);
    boost::array<char, 1024> buf;

    http_state<content_store_stream<std::stringstream>> state;
    bool header_written = false;

    while ( state ) {
        auto len = socket.read_some(boost::asio::buffer(buf), error);

        if ( error == boost::asio::error::eof ) {
            break;
        } else if ( error ) {
            throw boost::system::system_error(error);
        }

        state(buf.begin(), buf.begin() + len);

        if ( state.http_result() != 200 ) {
            throw std::runtime_error(std::string("http request error, expected code 200, but got ") + std::to_string(state.http_result()));
        }

        if ( state.is_header_finished() && !header_written ) {
            header_written = true;
            BOOST_LOG_TRIVIAL(debug) << L"recived response from server";
            BOOST_LOG_TRIVIAL(debug) << L"http header";
            BOOST_LOG_TRIVIAL(debug) << L"http status code " << state.http_result();
            for ( auto& kvp : state.header() ) {
                BOOST_LOG_TRIVIAL(debug) << kvp.first << ": " << kvp.second;
            }
        }
    }

    dlg_.unknown_progress(false);

    dlg_.progress(30);
    dlg_.info_msg(L"...patch data recived, processing...");
    BOOST_LOG_TRIVIAL(debug) << L"http content";
    BOOST_LOG_TRIVIAL(debug) << state.content().stream.str();

    BOOST_LOG_TRIVIAL(debug) << L"closing connection to server";
    socket.close();

    dlg_.progress(40);
    BOOST_LOG_TRIVIAL(debug) << L"parsing response";

    boost::property_tree::ptree version_info;
    boost::property_tree::json_parser::read_json((std::istringstream&)state.content().stream, version_info );

    int max_ver = 0;
    for ( auto& node : version_info ) {
        auto value = node.second.get_value<std::string>();
        BOOST_LOG_TRIVIAL(debug) << L"update on server: " << value;
        int c_ver;
        sscanf_s(value.c_str(), "updates/%d.update", &c_ver);
        if ( max_ver < c_ver ) {
            max_ver = c_ver;
            new_version_path = value;
        }
    }

    if ( max_ver > _version.build ) {
        dlg_.info_msg(L"...new version on server found...");
        BOOST_LOG_TRIVIAL(debug) << L"never version on server found: " << new_version_path;
    } else {
        dlg_.info_msg(L"...no new version on server found...");
        BOOST_LOG_TRIVIAL(debug) << L"application is up to date";
        new_version_path.clear();
    }

    dlg_.progress(50);

    return new_version_path;
#endif
}

std::string app::download_update(update_dialog& dlg_, std::string update_path_) {
    auto remote_server = std::to_string(_config.get<std::wstring>( L"update.server", L"homepages.thm.de" ));
    auto remote_path = std::to_string(_config.get<std::wstring>( L"update.path", L"/~hg14866/swtorla/" ));
    auto request = std::string("GET ") + remote_path + update_path_ + " HTTP/1.1\r\n"
                   "Host: " + remote_server + "\r\n"
                   "Connection: close\r\n"
                   "\r\n\r\n";
    std::string response;

    dlg_.progress(60);
    dlg_.info_msg(L"...locating patch file server...");
    BOOST_LOG_TRIVIAL(debug) << L"looking up remote server address for " << remote_server;
    boost::asio::ip::tcp::resolver tcp_lookup(_io_service);
    boost::asio::ip::tcp::resolver::query remote_query(remote_server, "http");
    auto list = tcp_lookup.resolve(remote_query);

    dlg_.progress(70);
    dlg_.info_msg(L"...connecting to patch file server...");
    BOOST_LOG_TRIVIAL(debug) << L"connecting to remote server";
    boost::asio::ip::tcp::socket socket(_io_service);
    boost::system::error_code error;
    boost::asio::ip::tcp::resolver::iterator lend;
    for ( ; lend != list; ++list ) {
        if ( !socket.connect(*list, error) ) {
            BOOST_LOG_TRIVIAL(debug) << L"connected to " << list->endpoint();
            break;
        }
    }

    if ( error ) {
        dlg_.info_msg(L"...can not connect to patch file server...");
        BOOST_LOG_TRIVIAL(error) << L"unable to connect to remote server";
        throw boost::system::system_error(error);
    }

    dlg_.progress(0);
    dlg_.info_msg(L"...requesting patch file from server...");
    BOOST_LOG_TRIVIAL(debug) << L"sending request to server " << request;
    if ( !socket.write_some(boost::asio::buffer(request), error) ) {
        BOOST_LOG_TRIVIAL(error) << L"failed to send request to server";
        throw boost::system::system_error(error);
    }

    dlg_.info_msg(L"...downloading patch file...");
    dlg_.unknown_progress(true);
    boost::array<char, 1024> buf;
    http_state<content_store_stream<std::ofstream>> state(content_store_stream<std::ofstream>( std::ofstream(PATCH_FILE_NAME, std::ios_base::out | std::ios_base::binary) ));
    bool header_written = false;

    while ( state ) {
        auto len = socket.read_some(boost::asio::buffer(buf), error);

        if ( error == boost::asio::error::eof ) {
            break;
        } else if ( error ) {
            throw boost::system::system_error(error);
        }

        state(buf.begin(), buf.begin() + len);

        if ( state.http_result() != 200 ) {
            throw std::runtime_error(std::string("http request error, expected code 200, but got ") + std::to_string(state.http_result()));
        }

        if ( state.is_header_finished() && !header_written ) {
            header_written = true;
            BOOST_LOG_TRIVIAL(debug) << L"recived response from server";
            BOOST_LOG_TRIVIAL(debug) << L"http header";
            BOOST_LOG_TRIVIAL(debug) << L"http status code " << state.http_result();
            for ( auto& kvp : state.header() ) {
                BOOST_LOG_TRIVIAL(debug) << kvp.first << ": " << kvp.second;
            }
        }
    }

    dlg_.unknown_progress(false);

    dlg_.info_msg(L"...download complete...");
    BOOST_LOG_TRIVIAL(debug) << L"closing connection to server";
    dlg_.progress(90);
    socket.close();

    return PATCH_FILE_NAME;
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

void app::remove_old_file() {
    wchar_t file_name[MAX_PATH];
    GetModuleFileName(nullptr, file_name, MAX_PATH);

    std::wstring temp_name(file_name);
    temp_name += L".old";

    BOOST_LOG_TRIVIAL(debug) << L"removing old file " << temp_name;

    while ( !DeleteFile(temp_name.c_str()) ) {
        if ( GetLastError() == ERROR_FILE_NOT_FOUND ) {
            break;
        }
    }

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

void app::send_crashreport( const char* path_ ) {
    // todo: add crash upload handler here
    (void)path_;
}

app::app(const char* caption_, const char* config_path_)
: _config_path(config_path_) {
    boost::log::add_file_log(boost::log::keywords::file_name = "app.log"
                            ,boost::log::keywords::format = "[%TimeStamp%]: %Message%"
                            ,boost::log::keywords::auto_flush = true);
    boost::log::core::get()->add_global_attribute("TimeStamp", boost::log::attributes::local_clock());
    
    send_crashreport(CRASH_FILE_NAME);

    _version = find_version_info(MAKEINTRESOURCEW(VS_VERSION_INFO));

    BOOST_LOG_TRIVIAL(info) << L"SW:ToR log analizer version " << _version.major << L"." << _version.minor << L"." << _version.patch << L" Build " << _version.build;

    read_config(_config_path);

    setup_from_config();

    INITCOMMONCONTROLSEX init =
    { sizeof( INITCOMMONCONTROLSEX ), /*ICC_PROGRESS_CLASS | ICC_TAB_CLASSES | ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES*/ 0xFFFFFFFF };
    InitCommonControlsEx(&init);

    remove_old_file();

    _log_reader.targets(_string_map, _char_list);
    _log_reader.processor([=](const combat_log_entry& e_) { log_entry_handler(e_); });

    _dir_watcher = dir_watcher(*this);

    _server = net_link_server(*this);
}


app::~app() {
    _server.stop();
    _dir_watcher.stop();
    _log_reader.stop();
    write_config(_config_path);
}


void app::operator()() {
    if ( _config.get<bool>( L"update.auto_check", true ) ) {
        if ( run_update_async() ) {
            return;
        }
    }

    _ui.reset(new main_ui(get_log_dir(), *this, _analizer, _string_map, _char_list));

    while ( _ui->handle_os_events() ) {
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
    return true;
}

void app::stop_tracking() {
    _dir_watcher.stop();
    _log_reader.stop();
    _analizer.clear();
}

std::future<bool> app::check_for_updates() {
    return run_update();
}

void app::on_new_log_file(const std::wstring& file_) {
    _log_reader.stop();
    _analizer.clear();
    _log_reader.start(_current_log_file, change_log_file(file_));
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

struct tm app::change_log_file(const std::wstring& file_, bool relative_ /*= true*/) {
    if ( !_current_log_file.empty() ) {
        if ( archive_log(_current_log_file) ) {
            remove_log(_current_log_file);
        }
    }

    if ( relative_ ) {
        auto log_path = _config.get<std::wstring>( L"log.path", L"" );
        _current_log_file = log_path + L"\\" + file_;
    } else {
        _current_log_file = file_;
    }

    auto info = get_date_from_file_name(_current_log_file);
    // zer out time stuff to make it a base value for the day
    info.tm_sec = 0;
    info.tm_min = 0;
    info.tm_hour = 0;
    info.tm_wday = 0;
    info.tm_yday = 0;
    info.tm_isdst = 0;

    return info;
    
    //BOOST_LOG_TRIVIAL(debug) << L"log file " << _current_log_file << " parsed into " << info.year << "." << info.month << "." << info.day << " " << info.hour << ":" << info.minute << ":" << info.second;
}

std::wstring app::get_archive_name_from_log_name(const std::wstring& name_) {
    return name_ + L".zip";
}

bool app::archive_log(const std::wstring& file_) {
#if 0
    if ( !_config.get<bool>( L"log.archive", false ) ) {
        return true;
    }

    bool archive_per_file = _config.get<bool>( L"log.archive_file_seperate", false );

    std::wstring arch_name;
    bool append = !archive_per_file;
    if ( archive_per_file ) {
        arch_name = get_archive_name_from_log_name(file_);
    } else {
        arch_name = _config.get<std::wstring>( L"log.archive_name", L"logs.zip" );
    }

    archive_zip zip(arch_name, archive_zip::open_mode::write, append);
    if ( !zip.is_open() ) {
        zip.open(arch_name, archive_zip::open_mode::write, false);
        if ( !zip.is_open() ) {
            BOOST_LOG_TRIVIAL(debug) << L"can not open archive " << arch_name << " for writing";
            return false;
        }
    }
    auto length = ::WideCharToMultiByte(CP_UTF8, 0, file_.c_str(), file_.length(), nullptr, 0, nullptr, nullptr);
    std::string mbname(length, ' ');
    ::WideCharToMultiByte(CP_UTF8, 0, file_.c_str(), file_.length(), const_cast<char*>( mbname.c_str() ), mbname.length(), nullptr, nullptr);

    auto last_slash = mbname.find_last_of('\\');
    std::string local_name;
    if ( last_slash != std::wstring::npos ) {
        local_name = mbname.substr(last_slash + 1);
    } else {
        local_name = mbname;
    }
    // TODO: get actual file time
    if ( !zip.create_file(local_name.c_str(), std::chrono::system_clock::now()) ) {
        BOOST_LOG_TRIVIAL(debug) << L"can not add log file " << local_name << L" (" << file_ << L") to archive " << arch_name;
        return false;
    }

    std::ifstream file(mbname, std::ios_base::in | std::ios_base::binary);
    std::array<char, 1024 * 4> buffer;
    while ( file.read(buffer.data(), buffer.size()) ) {
        zip.write_file(buffer.data(), buffer.size());
    }
    zip.write_file(buffer.data(), file.gcount());
    zip.close_file();
    return true;
#endif
    if ( !_config.get<bool>( L"log.archive", false ) ) {
        return true;
    }
    
    bool archive_per_file = _config.get<bool>( L"log.archive_file_seperate", false );

    std::wstring arch_name;
    bool append = !archive_per_file;
    if ( archive_per_file ) {
        arch_name = get_archive_name_from_log_name(file_);
    } else {
        arch_name = _config.get<std::wstring>( L"log.archive_name", L"logs.zip" );
    }

    STARTUPINFO si
    {};
    PROCESS_INFORMATION pi
    {};

    si.cb = sizeof( si );

    std::wstring command = L"7z.exe a ";
    command += L"\"";
    command += arch_name;
    command += L"\" \"";
    command += file_;
    command += L"\"";
    BOOST_LOG_TRIVIAL(debug) << L"running 7z.exe with " << command;
    CreateProcessW(nullptr, const_cast<wchar_t*>( command.c_str() ), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);

    return true;
}

void app::remove_log(const std::wstring& file_) {
    if ( !_config.get<bool>( L"log.remove", false ) ) {
        return;
    }

    unsigned a;
    for ( a = 0; a < 25; ++a ) {
        if ( FALSE == ::DeleteFileW(file_.c_str()) ) {
            BOOST_LOG_TRIVIAL(debug) << L"Deleting file " << file_ << " failed...";
            continue;
        }
        break;
    }
}

boost::asio::io_service& app::get_io_service() {
    return _io_service;
}

void app::on_connected_to_server(client_net_link* self_) {
    BOOST_LOG_TRIVIAL(debug) << L"void app::on_connected_to_server(" << self_ << L");";
    self_->register_at_server(_char_list[_current_char]);
}

void app::on_disconnected_from_server(client_net_link* self_) {
    BOOST_LOG_TRIVIAL(debug) << L"void app::on_connected_to_server(" << self_ << L");";
    // UPDATE ui?
}

void app::on_string_lookup(client_net_link* self_, string_id string_id_) {
    BOOST_LOG_TRIVIAL(debug) << L"void app::on_string_lookup(" << self_ << L", " << string_id_ << L");";
    // server requested a string look up
    auto ref = _string_map.find(string_id_);

    if ( ref != end(_string_map) ) {
        // send string to server
        self_->send_string_value(string_id_, ref->second);
    }
}

void app::on_string_info(client_net_link * self_, string_id string_id_, const std::wstring& string_) {
    BOOST_LOG_TRIVIAL(debug) << L"void app::on_string_info(" << self_ << L", " << string_id_ << L", " << string_ << L");";
    // just insert it, if it allready exists the current value is kept
    _string_map.insert(std::make_pair(string_id_, string_));
}

void app::on_combat_event(client_net_link * self_, const combat_log_entry& event_) {
    BOOST_LOG_TRIVIAL(debug) << L"void app::on_combat_event(" << self_ << L", " << &event_ << L");";
    // use s specialaized version of log_entry_handler to translate server name ids to local name ids
    //log_entry_handler(event_);
}

void app::on_set_name(client_net_link * self_, string_id name_id_, const std::wstring& name_) {
    BOOST_LOG_TRIVIAL(debug) << L"void app::on_set_name(" << self_ << L", " << name_id_ << L", " << name_ << L");";
    // find a free id
    auto local = _id_map.add(name_id_);
    // ensure we have enoug slots at char list
    _char_list.resize(std::max(local + 1, _char_list.size()));
    // store name
    _char_list[local] = name_;
}

void app::new_client(boost::asio::ip::tcp::socket* socket_) {
    BOOST_LOG_TRIVIAL(debug) << L"void app::new_client(" << socket_ << L");";
    std::unique_lock<decltype( _clients_guard )> lock(_clients_guard);
    _clients.push_back(std::make_unique<server_net_link>( *this, socket_ ));
}

void app::on_client_register(server_net_link * self_, const std::wstring& name_) {
    BOOST_LOG_TRIVIAL(debug) << L"void app::on_client_register(" << self_ << L", " << name_ << L");";
    auto at = std::find(begin(_char_list), end(_char_list), name_);
    if ( at == end(_char_list) ) {
        at = _char_list.push_back(name_);
    }
    auto id = std::distance(begin(_char_list), at);
    for ( auto& cl : _clients ) {
        cl->send_set_name(id, name_);
    }
}

void app::on_string_lookup(server_net_link* self_, string_id string_id_) {
    BOOST_LOG_TRIVIAL(debug) << L"void app::on_string_lookup(" << self_ << L", " << string_id_ << L");";
    auto ref = _string_map.find(string_id_);
    if ( ref == std::end(_string_map) ) {
        for ( auto& cl : _clients ) {
            cl->get_string_value(string_id_);
        }
    } else {
        self_->send_string_value(string_id_, ref->second);
    }
}

void app::on_string_info(server_net_link* self_, string_id string_id_, const std::wstring& string_) {
    BOOST_LOG_TRIVIAL(debug) << L"void app::on_string_lookup(" << self_ << L", " << string_id_ << L", " << string_ << L");";
    auto at = _string_map.insert(std::make_pair(string_id_, string_)).first;

    for ( auto& cl : _clients ) {
        if ( cl.get() != self_ ) {
            cl->send_string_value(at->first, at->second);
        }
    }
}

void app::on_combat_event(server_net_link* self_, const combat_log_entry& event_) {
    BOOST_LOG_TRIVIAL(debug) << L"void app::on_combat_event(" << self_ << L", " << &event_ << L");";
    // insert into combat table
    for ( auto& cl : _clients ) {
        if ( cl.get() != self_ ) {
            cl->send_combat_event(event_);
        }
    }
}

void app::on_client_disconnect(server_net_link* self_) {
    BOOST_LOG_TRIVIAL(debug) << L"void app::on_client_disconnect(" << self_ << L");";
    std::unique_lock<decltype( _clients_guard )> lock(_clients_guard);
    auto ref = find_if(begin(_clients), end(_clients), [=](const std::unique_ptr<server_net_link>& other_) { return self_ == other_.get(); });
    if ( ref != end(_clients) ) {
        _clients.erase(ref);
    }
}