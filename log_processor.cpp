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

void log_processor::on_stop_thread(stop_thread_event) {
}

void log_processor::on_open_log(const open_log_event& e_) {
    BOOST_LOG_TRIVIAL(debug) << L"[log_processor] reading file " << e_.path;
    _file_handle.reset(::CreateFileW(e_.path.c_str()
                                    , GENERIC_READ
                                    , FILE_SHARE_READ | FILE_SHARE_WRITE
                                    , nullptr
                                    , OPEN_EXISTING
                                    , 0//FILE_FLAG_OVERLAPPED// | FILE_FLAG_SEQUENTIAL_SCAN
                                    , nullptr)
                      , [](HANDLE file_) { ::CloseHandle(file_); });
    _stop = false;
}
void log_processor::on_close_log(close_log_event) {
    BOOST_LOG_TRIVIAL(debug) << L"[log_processor] file closed";
    _stop = true;
    _file_handle.reset();
}

void log_processor::handle_event(const any& a_) {
#define do_handle_event(type_,handler_) do_handle_event<type_>(a_,[=](const type_& e_) { handler_(e_); })
#define do_handle_event_e(event_) do_handle_event(event_##_event,on_##event_)
    if ( !do_handle_event_e(close_log)
      && !do_handle_event_e(open_log)
      && !do_handle_event_e(stop_thread) ) {
        on_unhandled_event(a_);
    }
}

char* log_processor::process_bytes(char* from_, char* to_) {
    auto start = from_;
    while ( from_ < to_ ) {
        auto le = std::find(from_, to_, '\n');

        if ( le == to_ ) {
            //BOOST_LOG_TRIVIAL(debug) << L"[log_processor] no line end found for : " << std::string(from_, to_);
            return std::copy(from_, to_, start);
        }

        //BOOST_LOG_TRIVIAL(debug) << L"[log_processor] parsing line: " << std::string(from_, le);

        if ( _string_map && _char_list ) {
            try {
                auto entry = parse_combat_log_line(from_, le, *_string_map, *_char_list);
                if ( _entry_processor ) {
                    _entry_processor(entry);
                }
                auto h = GetStdHandle(STD_OUTPUT_HANDLE);
                std::stringstream buf;
                buf << "Entry: "
                    << entry.time_index.hours
                    << ":"
                    << entry.time_index.minutes
                    << ":"
                    << entry.time_index.seconds
                    << "."
                    << entry.time_index.milseconds
                    << " src[ ";
                if ( entry.src <= _char_list->size() ) {
                    buf << ( *_char_list )[entry.src];
                } else {
                    buf << ( *_string_map )[entry.src];
                }
                buf << " ] dst[ ";
                if ( entry.dst <= _char_list->size() ) {
                    buf << ( *_char_list )[entry.dst];
                } else {
                    buf << ( *_string_map )[entry.dst];
                }
                buf << " ] spell[ "
                    << ( *_string_map )[entry.ability]
                    << " ]";

                buf << "\r\n";
                DWORD w;
                WriteConsoleA(h, buf.str().c_str(), buf.str().length(), &w, nullptr);
                //WriteFile(h,)
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
    if ( _stop ) {
        get_events(( HWND ) - 1, 0, 0, [=](const MSG& msg) { return msg.message != APP_EVENT; });
    } else {
        peek_events(( HWND ) - 1);
    }
    std::array<char, 1024 * 4> read_buffer;
    DWORD bytes_read;
    char* from = read_buffer.data();
    char* to = from + read_buffer.size();
    DWORD wait_ms = overlapped_min_wait_ms;
    
    while ( !_stop ) {
        if ( ::ReadFile(*_file_handle, from, to - from, &bytes_read, nullptr) ) {
            //BOOST_LOG_TRIVIAL(debug) << L"[log_processor] read compledet, got " << bytes_read << " bytes read";
            if ( bytes_read > 0 ) {
                wait_ms = std::max<DWORD>( wait_ms >> 1, overlapped_min_wait_ms );
                from = process_bytes(read_buffer.data(), from + bytes_read);
            } else {
                peek_events(( HWND ) - 1);
                ::Sleep(wait_ms);
                wait_ms = std::min<DWORD>( wait_ms + wait_ms, overlapped_max_wait_ms );
            }
        } else {
            if ( ERROR_HANDLE_EOF != GetLastError() ) {
                break;
            }
        }
        peek_events(( HWND ) - 1);
    }

    
}

log_processor::log_processor() {
    _stop = true;

    AllocConsole();
}

log_processor::~log_processor() {
    stop();
    FreeConsole();
}

void log_processor::start(const std::wstring& path) {
    post_event(open_log_event{ path });
}

void log_processor::stop() {
    post_event(close_log_event{});
}
