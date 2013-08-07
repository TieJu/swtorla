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

inline unsigned store_bit(char* blob_, unsigned bit_offset_, bool bit_) {
    blob_[bit_offset_ / 8] |= ( bit_ ? 1 : 0 ) << ( bit_offset_ % 8 );
    return bit_offset_ + 1;
}

inline unsigned store_bits(char* blob_, unsigned bit_offset_, char* src_, unsigned bits_) {
    unsigned bit = 0;
    for ( ; bit < bits_; ++bit ) {
        bit_offset_ = store_bit(blob_, bit_offset_, src_[bit / 8] & ( 1 << ( bit % 8 ) ));
    }
    return bit_offset_;
}

inline unsigned bit_pack_int(char* blob_,unsigned bit_offset_, bool value_) {
    bit_offset_ = store_bit(blob_, bit_offset_, 0);
    bit_offset_ = store_bit(blob_, bit_offset_, value_);
    return bit_offset_;
}

namespace detail {
template<typename IntType>
inline unsigned count_bits(IntType value_) {
    unsigned bits = 1;
    value_ >>= 1;
    while ( value_ ) {
        ++bits;
        value_ >>= 1;
    }
    return bits;
    // for some odd reaseon
    // for ( ; value_ >> bits; ++bits ) will cause infinite loops ....
}
}
template<typename IntType>
inline unsigned count_bits(IntType value_) {
    // needs explicit cast to unsigned, or sign extending will ruin your day with a infinite loops...
    return detail::count_bits(static_cast<typename std::make_unsigned<IntType>::type>( value_ ));
}

template<typename IntType>
inline unsigned bit_pack_int(char* blob_, unsigned bit_offset_, IntType value_) {
    auto bits = count_bits(value_) - 1;

    bit_offset_ = store_bits(blob_, bit_offset_, reinterpret_cast<char*>( &bits ), 6);
    bit_offset_ = store_bits(blob_, bit_offset_, reinterpret_cast<char*>( &value_ ), bits + 1);

    return bit_offset_;
}

template<typename IntType>
inline char* pack_int(char* from_, char* to_, IntType value_) {
    do {
        *from_ = char( value_ & 0x7F );
        value_ >>= 7;
        if ( value_ ) {
            *from_ |= 0x80;
        }
        ++from_;
    } while ( value_ && from_ < to_ );
    return from_;
}

inline char* pack_int(char* from_, char* to_, bool value_) {
    *from_ = value_ ? 1 : 0;
    return from_ + 1;
}

void app::log_entry_handler(const combat_log_entry& e_) {
    _analizer.add_entry(e_);

    // tell the ui the new current play, this might change on a relog (move this elsewere, do this once
    // if a new combat log is opened)
    if ( e_.effect_action == ssc_Event && e_.effect_type == ssc_EnterCombat ) {
        _ui->update_main_player(e_.src);
    }
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
    _dir_watcher.reset(new dir_watcher(log_path,*this));
    auto file = find_path_for_lates_log_file(log_path);
    change_log_file(file,false);
    _log_reader.start(_current_log_file);
    return true;
}

void app::stop_tracking() {
    _dir_watcher.reset();
    _log_reader.stop();
}

std::future<bool> app::check_for_updates() {
    return run_update();
}

void app::on_new_log_file(const std::wstring& file_) {
    _log_reader.stop();
    change_log_file(file_);
    _log_reader.start(_current_log_file);
}

void app::change_log_file(const std::wstring& file_, bool relative_ /*= true*/) {
    if ( !_current_log_file.empty() ) {
        archive_log(_current_log_file);
        remove_log(_current_log_file);
    }

    if ( relative_ ) {
        auto log_path = _config.get<std::wstring>( L"log.path", L"" );
        _current_log_file = log_path + L"\\" + file_;
    } else {
        _current_log_file = file_;
    }
}

void app::archive_log(const std::wstring& file_) {
    if ( !_config.get<bool>( L"log.archive", false ) ) {
        return;
    }
    // TODO: add code that stores the log file in an archive (.zip)
}

void app::remove_log(const std::wstring& file_) {
    if ( !_config.get<bool>( L"log.remove", false ) ) {
        return;
    }
    // TODO: add code that removes the log file from disc
}