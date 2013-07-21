#pragma once

#include "handle_wrap.h"

#include <windows.h>
#include <chrono>
#include <thread>
#include <array>
#include <vector>
#include <algorithm>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

class dir_watcher {
    enum {
        buffer_size = 1024 * 8
    };
    handle_wrap<HANDLE, INVALID_HANDLE_VALUE>   _file_handle;
    std::thread                                 _handler_thread;
    std::array<std::array<char, buffer_size>, 2>_buffer;

    void on_added_file(const wchar_t* begin_, const wchar_t* end_);
    void on_removed_file(const wchar_t* begin_, const wchar_t* end_);
    void on_modified_file(const wchar_t* begin_, const wchar_t* end_);
    void on_renamed_old_file(const wchar_t* begin_, const wchar_t* end_);
    void on_renamed_new_file(const wchar_t* begin_, const wchar_t* end_);

    void process_data(DWORD length_);

    void thread_entry(const std::wstring& name_);

    dir_watcher(const dir_watcher&);
    dir_watcher& operator=(const dir_watcher&);

public:
    dir_watcher(const std::wstring& path);
    ~dir_watcher();
};