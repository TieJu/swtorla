#include "log_processor.h"
#include <algorithm>

#include <boost/scope_exit.hpp>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

#include <cstddef>
#include <sstream>

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif


log_processor::state log_processor::wait(state state_) {
    std::unique_lock<std::mutex> lock(_sleep_mutex);
    while ( state_ == _next_state ) {
        _sleep_signal.wait( lock );
    }

    state_ = _next_state;

    return _next_state;
}

void log_processor::wake() {
    _sleep_signal.notify_all();
}

void log_processor::open_log(const std::wstring& path_) {
    BOOST_LOG_TRIVIAL(debug) << L"[log_processor] reading file " << path_;
    _file_handle.reset(::CreateFileW(path_.c_str()
                                    , GENERIC_READ
                                    , FILE_SHARE_READ | FILE_SHARE_WRITE
                                    , nullptr
                                    , OPEN_EXISTING
                                    , 0//FILE_FLAG_OVERLAPPED// | FILE_FLAG_SEQUENTIAL_SCAN
                                    , nullptr)
                      , [](HANDLE file_) { ::CloseHandle(file_); });
}

char* log_processor::process_bytes(char* from_, char* to_) {
    auto start = from_;
    while ( from_ < to_ ) {
        auto le = std::find(from_, to_, '\n');

        if ( le == to_ ) {
            return std::copy(from_, to_, start);
        }

        if ( _string_map && _char_list ) {
            try {
                auto entry = parse_combat_log_line(from_, le, *_string_map, *_char_list);
                if ( _entry_processor ) {
                    _entry_processor(entry);
                }
            } catch ( const std::exception &e ) {
                BOOST_LOG_TRIVIAL(error) << L"[log_processor] line parsing failed, because " << e.what() << ", line was: " << std::string(from_, le);
            } catch ( ... ) {
                BOOST_LOG_TRIVIAL(error) << L"[log_processor] line parsing failed, line was: " << std::string(from_, le);
            }
        }
        from_ = le + 1; // line end is \r\n
    }
    return start;
}

void log_processor::run() {
    state t_state = state::sleep;
    auto wait_ms = std::chrono::milliseconds(overlapped_min_wait_ms);

    std::array<char, 1024 * 4> read_buffer;
    DWORD bytes_read;
    char* from = read_buffer.data();
    char* to = from + read_buffer.size();

    for ( ;; ) {
        if ( t_state == state::sleep ) {
            _file_handle.reset();
            t_state = wait(t_state);
        } else {
            t_state = wait_for(t_state, wait_ms);
            wait_ms = std::min(wait_ms + wait_ms, std::chrono::milliseconds(overlapped_max_wait_ms));
        }

        if ( t_state == state::shutdown ) {
            break;
        }

        if ( t_state == state::open_file ) {
            std::lock_guard<std::mutex> lock(_sleep_mutex);
            open_log(_path);
            _next_state = state::processing;
        }

        if ( t_state == state::processing ) {
            for ( ;; ) {
                if ( ::ReadFile(*_file_handle, from, to - from, &bytes_read, nullptr) ) {
                    //BOOST_LOG_TRIVIAL(debug) << L"[log_processor] read compledet, got " << bytes_read << " bytes read";
                    if ( bytes_read > 0 ) {
                        wait_ms = std::max(wait_ms / 2, std::chrono::milliseconds(overlapped_min_wait_ms));
                        from = process_bytes(read_buffer.data(), from + bytes_read);
                        continue;
                    }
                }

                wait_ms = std::min(wait_ms + wait_ms, std::chrono::milliseconds(overlapped_max_wait_ms));
                break;
            }
        }
    }
}

log_processor::log_processor() {
    _next_state = state::sleep;
    _thread = std::thread([=]() {
        run();
    });
}

log_processor::~log_processor() {
    stop();
    _next_state = state::shutdown;
    wake();
    _thread.join();
}

void log_processor::start(const std::wstring& path) {
    std::lock_guard<std::mutex> lock(_sleep_mutex);
    _path = path;
    _next_state = state::open_file;
    wake();
}

void log_processor::stop() {
    std::lock_guard<std::mutex> lock(_sleep_mutex);
    _next_state = state::sleep;
    wake();
}
