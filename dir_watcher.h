#pragma once

#include "handle_wrap.h"

#include <windows.h>
#include <chrono>
#include <thread>
#include <array>
#include <vector>
#include <algorithm>

#include <boost/noncopyable.hpp>

class dir_watcher
    : boost::noncopyable {
public:
    typedef std::function<void ( const std::wstring& )>  callback;
private:
    enum {
        buffer_size = 1024 * 8
    };
    handle_wrap<HANDLE, INVALID_HANDLE_VALUE>   _file_handle;
    std::thread                                 _handler_thread;
    std::array<std::array<char, buffer_size>, 2>_buffer;
    std::vector<callback>                       _add_callbacks;

    void on_added_file(const wchar_t* begin_, const wchar_t* end_);
    void on_removed_file(const wchar_t* begin_, const wchar_t* end_);
    void on_modified_file(const wchar_t* begin_, const wchar_t* end_);
    void on_renamed_old_file(const wchar_t* begin_, const wchar_t* end_);
    void on_renamed_new_file(const wchar_t* begin_, const wchar_t* end_);

    void process_data(DWORD length_);

    void thread_entry(const std::wstring& name_);

public:
    dir_watcher(const std::wstring& path);
    ~dir_watcher();

    template<typename Clb>
    void add_handler(Clb clb_) {
        _add_callbacks.push_back(std::forward<Clb>( clb_ ));
    }
};