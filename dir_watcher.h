#pragma once

#include "handle_wrap.h"
#include "active.h"

#include <windows.h>
#include <chrono>
#include <thread>
#include <array>
#include <vector>
#include <algorithm>

#include <boost/noncopyable.hpp>

class app;

class dir_watcher
: public active<dir_watcher> {
private:
    enum {
        buffer_size = 1024 * 8
    };
    handle_wrap<HANDLE, INVALID_HANDLE_VALUE>   _file_handle;
    std::array<std::array<char, buffer_size>, 2>_buffer;
    app*                                        _app;
    std::wstring                                _name;

    void on_added_file(const wchar_t* begin_, const wchar_t* end_);
    void on_removed_file(const wchar_t* begin_, const wchar_t* end_);
    void on_modified_file(const wchar_t* begin_, const wchar_t* end_);
    void on_renamed_old_file(const wchar_t* begin_, const wchar_t* end_);
    void on_renamed_new_file(const wchar_t* begin_, const wchar_t* end_);

    void process_data(DWORD length_);

protected:
    friend class active<dir_watcher>;
    void run();

public:
    dir_watcher();
    explicit dir_watcher(app* app_);
    dir_watcher(dir_watcher&& other_);
    dir_watcher& operator=(dir_watcher&& other_);
    ~dir_watcher();

    void watch(const std::wstring& path_);
    void stop();
};