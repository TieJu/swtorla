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
#include "swtor_log_parser.h"

#include "event_thread.h"

class log_processor {
    enum {
        buffer_size             = 1024 * 8,
        overlapped_min_wait_ms  = 250,
        overlapped_max_wait_ms  = 2500,
    };
    enum class state {
        sleep,
        processing,
        open_file,
        shutdown,
    };
    std::thread                                 _thread;
    handle_wrap<HANDLE, INVALID_HANDLE_VALUE>   _file_handle;
    std::array<std::array<char, buffer_size>, 2>_buffer;
    string_to_id_string_map*                    _string_map;
    character_list*                             _char_list;
    std::function<void(const combat_log_entry&)>_entry_processor;
    std::wstring                                _path;
    state                                       _next_state;
    std::mutex                                  _sleep_mutex;
    std::condition_variable                     _sleep_signal;


    friend class event_thread<log_processor>;

    template<class Rep, class Period>
    state wait_for(state state_, const std::chrono::duration<Rep, Period>& rel_time_);
    state wait(state state_);
    void wake();

    void open_log(const std::wstring& path_);
    char* process_bytes(char* from_, char* to_);

protected:
    void run();

public:
    log_processor();
    ~log_processor();
    void targets(string_to_id_string_map& string_map_, character_list& char_list_) {
        _string_map = &string_map_;
        _char_list = &char_list_;
    }
    template<typename U>
    void processor(U v_) {
        _entry_processor = std::forward<U>( v_ );
    }
    void start(const std::wstring& path);
    void stop();
};