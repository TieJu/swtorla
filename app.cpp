#include "app.h"

#include <sstream>
#include <future>
#include <thread>
#include <tuple>

#include <boost/property_tree/json_parser.hpp>

#include <boost/array.hpp>


#include "to_string.h"
#include "http_state.h"

#include "path_finder.h"

#include "to_string.h"
#include "update_info_dialog.h"

#define PATCH_FILE_NAME "patch.bin"
#define WPATCH_FILE_NAME L"patch.bin"

#define CRASH_FILE_NAME "./crash.dump"

struct loaded_patch_data_event {};
struct loaded_patch_file_event {};
struct update_done_event {};
struct enter_main_window_event {};


std::wstring get_version_param(LPCVOID block, const wchar_t* entry) {
    UINT vLen;
    DWORD langD;
    BOOL retVal;

    LPVOID retbuf = NULL;

    static wchar_t fileEntry[256];

    swprintf_s(fileEntry, L"\\VarFileInfo\\Translation");
    retVal = VerQueryValue(block, fileEntry, &retbuf, &vLen);
    if ( retVal && vLen == 4 ) {
        memcpy(&langD, retbuf, 4);
        swprintf_s(fileEntry, L"\\StringFileInfo\\%02X%02X%02X%02X\\%s",
                 ( langD & 0xff00 ) >> 8, langD & 0xff, ( langD & 0xff000000 ) >> 24,
                 ( langD & 0xff0000 ) >> 16, entry);
    } else {
        swprintf_s(fileEntry, L"\\StringFileInfo\\%04X04B0\\%s",
                   GetUserDefaultLangID(), entry);
    }

    if ( VerQueryValue(block, fileEntry, &retbuf, &vLen ) ) {
        return reinterpret_cast<const wchar_t*>(retbuf);
    }
    return std::wstring();
}

program_version find_version_info() {
    program_version info{};

    auto h_version_res = FindResource(nullptr, MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION);

    if ( !h_version_res ) {
        return info;
    }

    auto h_loaded_version = LoadResource(nullptr, h_version_res);

    if ( !h_loaded_version ) {
        return info;
    }

    auto h_locked_version = LockResource(h_loaded_version);

    auto version = get_version_param(h_locked_version, L"ProductVersion");

    swscanf_s(version.c_str(), L"%d.%d.%d.Build%d", &info.major, &info.minor, &info.patch, &info.build);

    UnlockResource(h_loaded_version);
    FreeResource(h_loaded_version);

    return info;
}

int get_build_from_name(const std::string& name_) {
    int c_ver = 0;
    sscanf_s(name_.c_str(), "updates/%d.update", &c_ver);
    return c_ver;
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
    dlg.callback([=](dialog* dlg_, UINT msg_, WPARAM w_param_, LPARAM l_param_) {
        if ( msg_ == WM_CLOSE || msg_ == WM_DESTROY ) {
            ::PostQuitMessage(0);
        }
        return FALSE;
    });

    auto job = std::async(std::launch::async, [=, &dlg]() { return run_update_async_job(dlg); });

    bool result = false;

    MSG msg{};
    while ( job.wait_for(std::chrono::milliseconds(100)) != std::future_status::ready ) {
        while ( ::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE) ) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    ::PostQuitMessage(0);
    try {
        result = job.get();
    } catch ( const std::exception& e_ ) {
        BOOST_LOG_TRIVIAL(error) << L"run_async_job failed because: " << e_.what();
    }

    while ( ::GetMessageW(&msg, nullptr, 0, 0) ) {
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

    // tell the ui the new current play, this might change on a relog (move this elsewere, do this once
    // if a new combat log is opened)
    if ( e_.effect_action == ssc_Event && e_.effect_type == ssc_EnterCombat ) {
        _ui->update_main_player(e_.src);
    }

    //if ( _analizer.count_encounters() < 1 ) {
    //    return;
    //}

    ////if ( e_.effect_action == ssc_Event && e_.effect_type == ssc_ExitCombat ) {

    //auto player_records = _analizer.select_from<combat_log_entry>(_analizer.count_encounters() - 1, [=](const combat_log_entry& e_) {return e_; } )
    //    .where([=](const combat_log_entry& e_) {
    //        return e_.src == 0 && e_.src_minion == string_id(-1) && e_.ability != string_id(-1);
    //}).commit < std::vector < combat_log_entry >> ( );

    //long long total_heal = 0;
    //long long total_damage = 0;

    //auto player_heal = select_from<combat_log_entry_ex>( [=, &total_heal](const combat_log_entry& e_) {
    //    combat_log_entry_ex ex
    //    { e_ };
    //    ex.hits = 1;
    //    ex.crits = ex.was_crit_effect;
    //    ex.misses = 0;
    //    total_heal += ex.effect_value;
    //    return ex;
    //}, player_records )
    //    .where([=](const combat_log_entry& e_) {
    //        return e_.effect_action == ssc_ApplyEffect && e_.effect_type == ssc_Heal;
    //}).group_by([=](const combat_log_entry_ex& lhs_, const combat_log_entry_ex& rhs_) {
    //    return lhs_.ability == rhs_.ability;
    //}, [=](const combat_log_entry_ex& lhs_, const combat_log_entry_ex& rhs_) {
    //    combat_log_entry_ex res = lhs_;
    //    res.effect_value += rhs_.effect_value;
    //    res.effect_value2 += rhs_.effect_value2;
    //    res.hits += rhs_.hits;
    //    res.crits += rhs_.crits;
    //    res.misses += rhs_.misses;
    //    return res;
    //}).order_by([=](const combat_log_entry_ex& lhs_, const combat_log_entry_ex& rhs_) {
    //    return lhs_.effect_value < rhs_.effect_value;
    //}).commit < std::vector < combat_log_entry_ex >> ( );

    //auto player_damage = select_from<combat_log_entry_ex>( [=, &total_damage](const combat_log_entry& e_) {
    //    combat_log_entry_ex ex
    //    { e_ };
    //    ex.hits = 1;
    //    ex.crits = ex.was_crit_effect;
    //    ex.misses = ex.effect_value == 0;
    //    total_damage += ex.effect_value;
    //    return ex;
    //}, player_records )
    //    .where([=](const combat_log_entry& e_) {
    //        return e_.effect_action == ssc_ApplyEffect && e_.effect_type == ssc_Damage;
    //}).group_by([=](const combat_log_entry_ex& lhs_, const combat_log_entry_ex& rhs_) {
    //    return lhs_.ability == rhs_.ability;
    //}, [=](const combat_log_entry_ex& lhs_, const combat_log_entry_ex& rhs_) {
    //    combat_log_entry_ex res = lhs_;
    //    res.effect_value += rhs_.effect_value;
    //    res.effect_value2 += rhs_.effect_value2;
    //    res.hits += rhs_.hits;
    //    res.crits += rhs_.crits;
    //    res.misses += rhs_.misses;
    //    return res;
    //}).order_by([=](const combat_log_entry_ex& lhs_, const combat_log_entry_ex& rhs_) {
    //    return lhs_.effect_value < rhs_.effect_value;
    //}).commit < std::vector < combat_log_entry_ex >> ( );
    
    //if ( e_.effect_action == ssc_Event && e_.effect_type == ssc_ExitCombat ) {
    //    // todo: let a sperate thread do this every 250ms?
    //    for ( const auto& row : player_damage ) {
    //        update_player_combat_stat_event event;

    //        const auto& value = _string_map[row.ability];
    //        const std::locale locale("");
    //        typedef std::codecvt<char, wchar_t, std::mbstate_t> converter_type;
    //        const auto &converter = std::use_facet<converter_type>( locale );
    //        std::vector<wchar_t> to(value.length() * converter.max_length());
    //        std::mbstate_t state;
    //        const char* from_next;
    //        wchar_t* to_next;
    //        const auto result = converter.out(state
    //                                          , value.data()
    //                                          , value.data() + value.length()
    //                                          , from_next
    //                                          , to.data()
    //                                          , to.data() + to.size()
    //                                          , to_next);
    //        if ( result == converter_type::ok || result == converter_type::noconv ) {
    //            event.name.assign(to.data(), to_next);
    //        }

    //        event.total = total_damage;
    //        event.value = row.effect_value;

    //        _ui->send(event);
    //    }
    //}

    /*if ( e_.effect_action == ssc_Event && e_.effect_type == ssc_ExitCombat ) {
        
        BOOST_LOG_TRIVIAL(debug) << L"encounter ended:";
        BOOST_LOG_TRIVIAL(debug) << L"char " << _char_list[0] << L" did " << total_damage << L" dmg";
        BOOST_LOG_TRIVIAL(debug) << L"char " << _char_list[0] << L" did " << total_heal << L" healing";
        BOOST_LOG_TRIVIAL(debug) << L"dmg breakdown by ability:";
        for ( const auto& row : player_damage ) {
            BOOST_LOG_TRIVIAL(debug) << L"ability: " << _string_map[row.ability] << L" did " << row.effect_value << L" dmg, through " << row.hits << L" attacks, " << row.crits << L" where crits and " << row.misses << L" missed";
        }
        BOOST_LOG_TRIVIAL(debug) << L"heal breakdown by ability:";
        for ( const auto& row : player_heal ) {
            BOOST_LOG_TRIVIAL(debug) << L"ability: " << _string_map[row.ability] << L" did " << row.effect_value << L" healing, through " << row.hits << L" applications, " << row.crits << L" where crits";
        }
    }*/
}

std::string app::check_update(update_dialog& dlg_) {
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

void app::send_crashreport(const char* path_) {
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

    _version = find_version_info();

    BOOST_LOG_TRIVIAL(info) << L"SW:ToR log analizer version " << _version.major << L"." << _version.minor << L"." << _version.patch << L" Build " << _version.build;

    read_config(_config_path);

    setup_from_config();

    INITCOMMONCONTROLSEX init =
    { sizeof( INITCOMMONCONTROLSEX ), /*ICC_PROGRESS_CLASS | ICC_TAB_CLASSES | ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES*/ 0xFFFFFFFF };
    InitCommonControlsEx(&init);

    remove_old_file();

    _log_reader.targets(_string_map, _char_list);
    _log_reader.processor([=](const combat_log_entry& e_) { log_entry_handler(e_); });
}


app::~app() {
    _log_reader.stop();
    write_config(_config_path);

    BOOST_LOG_TRIVIAL(debug) << L"string table:";
    for ( auto& ent : _string_map ) {
        if ( ent.second.empty() ) {
            continue;
        }
        BOOST_LOG_TRIVIAL(debug) << L"[" << ent.first << "] = " << ent.second;
    }
}


void app::operator()() {
    if ( _config.get<bool>( L"update.auto_check", true ) ) {
        if ( run_update_async() ) {
            return;
        }
    }

    _ui.reset(new main_ui(get_log_dir(), *this));

    _ui->reciver<check_update_event>( [=](check_update_event e_) {
        *e_.target = std::move(run_update());
    } );

    _ui->send(set_analizer_event
    { &_analizer, &_string_map, &_char_list });

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
    _dir_watcher.reset(new dir_watcher(log_path));
    _dir_watcher->add_handler([=](const std::wstring& file_) {
        _log_reader.stop();
        _log_reader.start(log_path + L"\\" + file_);
    });
    auto file = find_path_for_lates_log_file(log_path);
    _log_reader.start(file);
    return true;
}

void app::stop_tracking() {
    _dir_watcher.reset();
    _log_reader.stop();
}