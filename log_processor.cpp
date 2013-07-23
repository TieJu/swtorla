#include "log_processor.h"

#include <boost/scope_exit.hpp>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

void log_processor::process_bytes(const char* from_, const char* to_) {
    BOOST_LOG_TRIVIAL(error) << L"[log_processor] file read data: " << std::string(from_, to_);
}

void log_processor::thread_entry() {
    BOOST_LOG_TRIVIAL(error) << L"[log_processor] log processor thread started";
    std::array<char, 1024 * 4> read_buffer;
    OVERLAPPED o{};
    o.hEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
    DWORD bytes_read;
    BOOST_SCOPE_EXIT_ALL(= ) {
        ::CloseHandle(o.hEvent);
    };
    for ( ;; ) {
        BOOST_LOG_TRIVIAL(error) << L"[log_processor] waiting for sync event";
        ::WaitForSingleObject(*_sync_event, INFINITE);
        if ( _file_handle.empty() ) {
            BOOST_LOG_TRIVIAL(error) << L"[log_processor] file handle is empty, exit condition";
            break;
        }

        for ( ;; ) {
            BOOST_LOG_TRIVIAL(error) << L"[log_processor] overlapped read submited";
            ::ReadFile(*_file_handle, read_buffer.data(), read_buffer.size(), nullptr, &o);

            BOOST_LOG_TRIVIAL(error) << L"[log_processor] waiting for read to complete";
            if ( TRUE == ::GetOverlappedResult(*_file_handle, &o, &bytes_read, TRUE) ) {
                BOOST_LOG_TRIVIAL(error) << L"[log_processor] read compledet, got " << bytes_read << " bytes read";
                if ( bytes_read > 0 ) {
                    size_t offset = o.Offset | size_t(o.OffsetHigh) << 32;
                    offset += bytes_read;
                    o.Offset = offset;
                    o.OffsetHigh = offset >> 32;
                    process_bytes(read_buffer.data(), read_buffer.data() + bytes_read);
                } else {
                    if ( _file_handle.empty() ) {
                        BOOST_LOG_TRIVIAL(error) << L"[log_processor] stop reading, file handle is empty";
                        break;
                    } else {
                        BOOST_LOG_TRIVIAL(error) << L"[log_processor] error state, stoping";
                        throw std::runtime_error("GetOverlappedResult failed");
                    }
                }

                ::ResetEvent(o.hEvent);
            } else {
                auto ec = GetLastError();
                if ( ec == ERROR_HANDLE_EOF ) {
                    BOOST_LOG_TRIVIAL(error) << L"[log_processor] eof reached, waiting for 500 ms";
                    ::Sleep(500);
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

    _handler_thread = std::thread([=]() {
        try {
            thread_entry();
        } catch ( const std::runtime_error& e ) {
            BOOST_LOG_TRIVIAL(error) << L"log processing faile, because " << e.what() << ", ending reader thread";
        }
    });
}

log_processor::~log_processor() {
    ::ResetEvent(*_sync_event);
    _file_handle.reset();
    ::SetEvent(*_sync_event);
    _handler_thread.join();
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
    ::SetEvent(*_sync_event);

}

void log_processor::stop() {
    ::ResetEvent(*_sync_event);
    _file_handle.reset();
}
