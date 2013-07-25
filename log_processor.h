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

struct combat_log_entry_event {
    combat_log_entry    entry;
};

struct open_log_event {
    std::wstring        path;
};

struct close_log_event {

};

class log_processor
: public event_thread<log_processor> {
    enum {
        buffer_size = 1024 * 8,
        overlapped_min_wait_ms = 250,
        overlapped_max_wait_ms = 5000,
    };
    handle_wrap<HANDLE, INVALID_HANDLE_VALUE>   _file_handle;
    std::array<std::array<char, buffer_size>, 2>_buffer;
    string_to_id_string_map*                    _string_map;
    character_list*                             _char_list;
    bool                                        _stop;
    std::function<void(const combat_log_entry&)>_entry_processor;

    friend class event_thread<log_processor>;

    void on_unhandled_event(const any& a_) {}
    void on_stop_thread(stop_thread_event);
    void on_open_log(const open_log_event& e_);
    void on_close_log(close_log_event);
    void handle_event(const any& a_);
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