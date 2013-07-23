#include "log_processor.h"

#include "swtor_log_parser.h"

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

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

char* log_processor::process_bytes(char* from_, char* to_) {
    auto start = from_;
    while ( from_ < to_ ) {
        auto le = std::find(from_, to_, '\r');

        if ( le == to_ ) {
            BOOST_LOG_TRIVIAL(debug) << L"[log_processor] no line end found for : " << std::string(from_, to_);
            return std::copy(from_, to_, start);
        }

        BOOST_LOG_TRIVIAL(debug) << L"[log_processor] parsing line: " << std::string(from_, le);
        //auto entry = parse_combat_log_line(from_, le);
        from_ = le + 2; // line end is \r\n
    }
    return start;
}

void log_processor::thread_entry() {
    BOOST_LOG_TRIVIAL(debug) << L"[log_processor] log processor thread started";
    std::array<char, 1024 * 4> read_buffer;
    DWORD bytes_read;
    char* from = read_buffer.data();
    char* to = from + read_buffer.size();
    DWORD wait_ms = 100;
    for ( ;; ) {
        BOOST_LOG_TRIVIAL(debug) << L"[log_processor] waiting for sync event";
        ::WaitForSingleObject(*_sync_event, INFINITE);
        if ( _file_handle.empty() ) {
            BOOST_LOG_TRIVIAL(error) << L"[log_processor] file handle is empty, exit condition";
            break;
        }

        for ( ;; ) {
            BOOST_LOG_TRIVIAL(debug) << L"[log_processor] overlapped read submited";
            if ( FALSE == ::ReadFile(*_file_handle, from, to - from, nullptr, &_overlapped) ) {
                auto ec = GetLastError();
                if ( ERROR_OPERATION_ABORTED == ec || ERROR_INVALID_HANDLE == ec ) {
                    break;
                }
            }

            BOOST_LOG_TRIVIAL(debug) << L"[log_processor] waiting for read to complete";
            if ( TRUE == ::GetOverlappedResult(*_file_handle, &_overlapped, &bytes_read, TRUE) ) {
                BOOST_LOG_TRIVIAL(debug) << L"[log_processor] read compledet, got " << bytes_read << " bytes read";
                ::ResetEvent(_overlapped.hEvent);
                wait_ms = std::max<DWORD>( wait_ms >> 1, 50 );
                if ( bytes_read > 0 ) {
                    LARGE_INTEGER offset;
                    offset.LowPart = _overlapped.Offset;
                    offset.HighPart = _overlapped.OffsetHigh;
                    offset.QuadPart += bytes_read;
                    _overlapped.Offset = offset.LowPart;
                    _overlapped.OffsetHigh = offset.HighPart;
                    from = process_bytes(read_buffer.data(), from + bytes_read);
                } else {
                    if ( _file_handle.empty() ) {
                        BOOST_LOG_TRIVIAL(debug) << L"[log_processor] stop reading, file handle is empty";
                        break;
                    } else {
                        BOOST_LOG_TRIVIAL(error) << L"[log_processor] error state, stoping";
                        throw std::runtime_error("GetOverlappedResult failed");
                    }
                }

            } else {
                auto ec = GetLastError();
                if ( ec == ERROR_HANDLE_EOF ) {
                    BOOST_LOG_TRIVIAL(debug) << L"[log_processor] eof reached, waiting for " << wait_ms << L" ms";
                    ::Sleep(wait_ms);
                    wait_ms = std::min<DWORD>(wait_ms + wait_ms, 1500);
                    continue;
                }
                BOOST_LOG_TRIVIAL(error) << L"[log_processor] read error, stoping";
                break;
            }
        }
    }
}

log_processor::log_processor() {
    _sync_event.reset(::CreateEventW(nullptr, TRUE, FALSE, nullptr), [](HANDLE h_) {CloseHandle(h_); });
    _overlapped.hEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);

    _handler_thread = std::thread([=]() {
        try {
            thread_entry();
        } catch ( const std::runtime_error& e ) {
            BOOST_LOG_TRIVIAL(error) << L"log processing faile, because " << e.what() << ", ending reader thread";
        }
    });
}

log_processor::~log_processor() {
    stop();
    ::SetEvent(*_sync_event);
    ::SetEvent(_overlapped.hEvent);
    _handler_thread.join();
    CloseHandle(_overlapped.hEvent);
}

void log_processor::start(const std::wstring& path) {
    _file_handle.reset(::CreateFileW(path.c_str()
                                    , GENERIC_READ
                                    , FILE_SHARE_READ | FILE_SHARE_WRITE
                                    , nullptr
                                    , OPEN_EXISTING
                                    , FILE_FLAG_OVERLAPPED// | FILE_FLAG_SEQUENTIAL_SCAN
                                    , nullptr)
                      , [](HANDLE file_) { ::CloseHandle(file_); });
    _overlapped.Offset = 0;
    _overlapped.OffsetHigh = 0;
    ::SetEvent(*_sync_event);
    ::ResetEvent(_overlapped.hEvent);
}

void log_processor::stop() {
    ::ResetEvent(*_sync_event);
    auto h = *_file_handle;
    _file_handle.reset();
    ::CancelIoEx(h, &_overlapped);
    ::SetEvent(_overlapped.hEvent);
}
