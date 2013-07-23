#pragma once

#include "handle_wrap.h"

#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <fstream>
#include <array>

#include <inplace_any.h>

#include <Windows.h>

#include <boost/noncopyable.hpp>

class log_processor
    : boost::noncopyable {
    enum {
        buffer_size = 1024 * 8
    };
    handle_wrap<HANDLE, INVALID_HANDLE_VALUE>   _file_handle;
    handle_wrap<HANDLE, nullptr>                _sync_event;
    std::array<std::array<char, buffer_size>, 2>_buffer;
    std::thread                                 _handler_thread; 
    OVERLAPPED                                  _overlapped;

    char* process_bytes(char* from_, char* to_);
    void thread_entry();
public:
    log_processor();
    ~log_processor();
    void start(const std::wstring& path);
    void stop();
};
// use the same technique as for dir watcher
/*
class log_processor {
    struct stop_signal {
    };
    struct start_signal {
        std::wstring path;
    };
    struct exit_signal {};

    typedef tj::inplace_any<sizeof( start_signal )> any_command;
    typedef std::array<any_command, 3>              command_buffer;

    std::thread             _worker;
    std::mutex              _sync;
    std::condition_variable _sync_signal;
    command_buffer          _commands;
    size_t                  _read_index;
    size_t                  _write_index;

    bool parse_line(std::wifstream& stream_);

    bool check_stop();

    void run_stop();
    void run_start(const std::string& path_) {
        std::wifstream file(path_);

        if ( file.is_open() ) {
            bool run = true;
            for ( ;; ) {
                while ( run && parse_line(file) ) {
                    run = check_stop();
                }
                if ( !run ) {
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }


    void run() {
        for ( ;; ) {
            std::lock_guard<std::mutex> lock(_sync);
            while ( !_avail ) {
                _sync_signal.wait(lock);
            }

            if ( tj::any_cast<stop_signal>( &_next_command ) ) {
            } else if ( tj::any_cast<exit_signal>( &_next_command ) ) {
                break;
            } else if ( tj::any_cast<start_signal>( &_next_command ) ) {
                run_start(tj::any_cast<start_signal>( &_next_command )->path);
            }
        }
    }

public:
    log_processor();

    void stop() {
    }
    void start(const std::wstring& log_) {
        std::lock_guard<std::mutex> lock(_sync);
        _next_command = start_signal{log_};
        _sync_signal.notify_one();
    }
};*/