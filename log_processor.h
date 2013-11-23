#pragma once

#include "handle_wrap.h"
#include "active.h"

#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <fstream>
#include <array>
#include <chrono>

#include <inplace_any.h>

#include <Windows.h>

#include <boost/noncopyable.hpp>
#include "swtor_log_parser.h"

#include "event_thread.h"

class app;

class log_processor {
    enum {
        buffer_size             = 1024 * 2
    };

    handle_wrap<HANDLE, INVALID_HANDLE_VALUE>   _file_handle;
    std::array<char, buffer_size>               _buffer;
    std::chrono::system_clock::time_point       _base_time;
    char*                                       _from;
    app*                                        _cl;

    void open_log(const std::wstring& path_);
    char* process_bytes(char* from_, char* to_);

public:
    log_processor();
    explicit log_processor( app* app_ );
    log_processor( log_processor&& other_ );
    log_processor& operator=( log_processor&& other_ );

    void start( const std::wstring& path, std::chrono::system_clock::time_point base_time_ );
    void stop();

    void operator()();
};