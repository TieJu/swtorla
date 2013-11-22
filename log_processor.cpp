#include "app.h"
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

        if ( _cl ) {
            try {
                auto entry = parse_combat_log_line(from_, le, _cl->get_string_map(), _cl->get_char_list(), _base_time);
                _cl->log_entry_handler( entry );
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

log_processor::log_processor( )
: log_processor( nullptr ) {
}

log_processor::log_processor( app* app_ )
: _cl( app_ ) {
    _from = _buffer.data( );
}

log_processor::log_processor( log_processor&& other_ )
: log_processor( ) {
    *this = std::move( other_ );
}

log_processor& log_processor::operator=( log_processor&& other_ ) {
    auto relative_from = other_._from - other_._buffer.data( );

    _file_handle = std::move( other_._file_handle );
    _buffer = std::move( other_._buffer );
    _base_time = std::move( other_._base_time );
    _from = _buffer.data( ) + relative_from;
    _cl = std::move( other_._cl );
    return *this;
}

void log_processor::start( const std::wstring& path, std::chrono::system_clock::time_point base_time_ ) {
    _base_time = base_time_;
    open_log( path );
}

void log_processor::stop( ) {
    _file_handle.reset();
}

void log_processor::operator()( ) {
    if ( !_file_handle ) {
        return;
    }

    DWORD bytes_read;
    auto to = _buffer.data() + _buffer.size();
    while ( ::ReadFile( *_file_handle, _from, to - _from, &bytes_read, nullptr ) ) {
        //BOOST_LOG_TRIVIAL(debug) << L"[log_processor] read compledet, got " << bytes_read << " bytes read";
        if ( bytes_read > 0 ) {
            _from = process_bytes( _buffer.data( ), _from + bytes_read );
        } else {
            break;
        }
    }
}