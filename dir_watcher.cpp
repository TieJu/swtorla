#include "app.h"

#include <boost/scope_exit.hpp>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

void dir_watcher::on_added_file(const wchar_t* begin_, const wchar_t* end_) {
    std::wstring name(begin_, end_);
    BOOST_LOG_TRIVIAL(debug) << L"added file " << name;
    _app->on_new_log_file(name);
}
void dir_watcher::on_removed_file(const wchar_t* begin_, const wchar_t* end_) {
    BOOST_LOG_TRIVIAL(debug) << L"removed file " << std::wstring(begin_, end_);
}
void dir_watcher::on_modified_file(const wchar_t* begin_, const wchar_t* end_) {
    BOOST_LOG_TRIVIAL(debug) << L"modified file " << std::wstring(begin_, end_);
}
void dir_watcher::on_renamed_old_file(const wchar_t* begin_, const wchar_t* end_) {
    BOOST_LOG_TRIVIAL(debug) << L"renamed file, old name " << std::wstring(begin_, end_);
}
void dir_watcher::on_renamed_new_file(const wchar_t* begin_, const wchar_t* end_) {
    BOOST_LOG_TRIVIAL(debug) << L"renamed file, new name " << std::wstring(begin_, end_);
}

void dir_watcher::process_data(DWORD length_) {
    if ( length_ < 1 ) {
        return;
    }
    
    BOOST_LOG_TRIVIAL(debug) << L"processing " << length_ << " bytes of change data";

    auto at = _buffer[1].data();
    auto end = at + length_;

    while ( at < end ) {
        auto info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>( at );

        auto name_begin = info->FileName;
        auto name_end = name_begin + ( info->FileNameLength / sizeof(info->FileName[0]) );

        switch ( info->Action ) {
        case FILE_ACTION_ADDED: on_added_file(name_begin, name_end); break;
        case FILE_ACTION_REMOVED: on_removed_file(name_begin, name_end); break;
        case FILE_ACTION_MODIFIED: on_modified_file(name_begin, name_end); break;
        case FILE_ACTION_RENAMED_OLD_NAME: on_renamed_old_file(name_begin, name_end); break;
        case FILE_ACTION_RENAMED_NEW_NAME: on_renamed_new_file(name_begin, name_end); break;
        }

        at += info->NextEntryOffset;

        if ( !info->NextEntryOffset ) {
            break;
        }
    }
}
#if 0
void dir_watcher::thread_entry(const std::wstring& name_) {
    BOOST_LOG_TRIVIAL(debug) << L"creating handle to observed directory " << name_;
    _file_handle.reset(CreateFile(name_.c_str()
                                , FILE_LIST_DIRECTORY
                                , FILE_SHARE_READ
                                | FILE_SHARE_WRITE
                                | FILE_SHARE_DELETE
                                , NULL
                                , OPEN_EXISTING
                                , FILE_FLAG_BACKUP_SEMANTICS
                                | FILE_FLAG_OVERLAPPED
                                , NULL)
                                // wrapper needed, close handle returns BOOL ....
                                , [](HANDLE h) { CloseHandle(h); });

    if ( _file_handle.empty() ) {
        BOOST_LOG_TRIVIAL(error) << L"unable to create handle to directory";
        throw std::runtime_error("CreateFile failed!");
    }
    BOOST_SCOPE_EXIT_ALL(= ) {
        BOOST_LOG_TRIVIAL(debug) << L"releasing directory handle";
        _file_handle.reset();
    };

    OVERLAPPED state{};
    DWORD read_bytes;
    const DWORD filter_mask = /*FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE |*/ FILE_NOTIFY_CHANGE_FILE_NAME;

    BOOST_LOG_TRIVIAL(debug) << L"creating sync event";
    state.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if ( !state.hEvent ) {
        BOOST_LOG_TRIVIAL(error) << L"unable to create sync event";
        throw std::runtime_error("CreateEvent returned null");
    }
    BOOST_SCOPE_EXIT_ALL(= ) {
        BOOST_LOG_TRIVIAL(debug) << L"releasing sync event";
        CloseHandle(state.hEvent);
    };

    BOOST_LOG_TRIVIAL(debug) << L"observing path";
    if ( !ReadDirectoryChangesW(*_file_handle, _buffer[0].data(), (DWORD)_buffer[0].size(), FALSE, filter_mask, nullptr, &state, nullptr) ) {
        BOOST_LOG_TRIVIAL(error) << L"observing failed";
        throw std::runtime_error("ReadDirectoryChangesW failed!");
    }

    for ( ;; ) {
        BOOST_LOG_TRIVIAL(debug) << L"waiting for changes";
        if ( TRUE == ::GetOverlappedResult(*_file_handle, &state, &read_bytes, TRUE) ) {
            if ( read_bytes > 0 ) {
                BOOST_LOG_TRIVIAL(debug) << read_bytes << L" bytes of change data recived";
                std::copy(begin(_buffer[0]), begin(_buffer[0]) + read_bytes, begin(_buffer[1]));
            }

            ResetEvent(state.hEvent);
            BOOST_LOG_TRIVIAL(debug) << L"observing path";
            if ( !ReadDirectoryChangesW(*_file_handle, _buffer[0].data(), (DWORD)_buffer[0].size(), FALSE, filter_mask, nullptr, &state, nullptr) ) {
                if ( !_file_handle.empty() ) {
                    BOOST_LOG_TRIVIAL(error) << L"observing failed";
                    throw std::runtime_error("ReadDirectoryChangesW failed!");
                } else {
                    BOOST_LOG_TRIVIAL(debug) << L"observing failed, handle was closed (termination request)";
                    break;
                }
            }

            process_data(read_bytes);
        } else {
            break;
        }
    }

}
#endif
void dir_watcher::run() {
    state rs = state::sleep;
    const DWORD filter_mask = /*FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE |*/ FILE_NOTIFY_CHANGE_FILE_NAME;
    OVERLAPPED ostate{};
    DWORD read_bytes = 0;

    ostate.hEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
    BOOST_SCOPE_EXIT_ALL(= ) {
        ::CloseHandle(ostate.hEvent);
    };

    for ( ;; ) {
        if ( state::sleep == rs || state::shutdown == rs ) {
            _file_handle.reset();
        }

        if ( state::shutdown == rs ) {
            break;
        }

        rs = wait(rs);

        if ( state::init == rs ) {
            BOOST_LOG_TRIVIAL(debug) << L"creating handle to observed directory " << _name;
            _file_handle.reset(CreateFile(_name.c_str()
                                         ,FILE_LIST_DIRECTORY
                                         ,FILE_SHARE_READ
                                         |FILE_SHARE_WRITE
                                         |FILE_SHARE_DELETE
                                         ,NULL
                                         ,OPEN_EXISTING
                                         ,FILE_FLAG_BACKUP_SEMANTICS
                                         |FILE_FLAG_OVERLAPPED
                                         ,NULL)
                                // wrapper needed, close handle returns BOOL ....
                                , [](HANDLE h) { CloseHandle(h); });

            if ( !_file_handle ) {
                BOOST_LOG_TRIVIAL(error) << L"unable to create handle to directory";
                change_state(state::sleep);
                continue;
            }
            rs = state::run;
            change_state(state::run);
        }

        while ( state::run == rs ) {
            if ( !ReadDirectoryChangesW(*_file_handle, _buffer[0].data(), (DWORD)_buffer[0].size(), FALSE, filter_mask, nullptr, &ostate, nullptr) ) {
                BOOST_LOG_TRIVIAL(error) << L"ReadDirectoryChangesW failed";
                change_state(state::sleep);
                break;
            }
            
            process_data(read_bytes);
            
            if ( TRUE == ::GetOverlappedResult(*_file_handle, &ostate, &read_bytes, TRUE) ) {
                if ( read_bytes > 0 ) {
                    BOOST_LOG_TRIVIAL(debug) << read_bytes << L" bytes of change data recived";
                    std::copy(begin(_buffer[0]), begin(_buffer[0]) + read_bytes, begin(_buffer[1]));
                }
            } else {
                BOOST_LOG_TRIVIAL(error) << L"GetOverlappedResult failed";
                change_state(state::sleep);
                break;
            }

            rs = wait_for(rs, std::chrono::milliseconds(25));
        }
    }
}

dir_watcher::dir_watcher()
    : _app(nullptr) {
}

dir_watcher::dir_watcher(app* app_)
    : _app(app_) {
}

dir_watcher::dir_watcher(dir_watcher && other_)
    : dir_watcher() {
    *this = std::move(other_);
}

dir_watcher& dir_watcher::operator=(dir_watcher&& other_) {
    active<dir_watcher>::operator=( std::move(other_) );
    _file_handle = std::move(other_._file_handle);
    _buffer = std::move(other_._buffer);
    _app = std::move(other_._app);
    _name = std::move(other_._name);
    return *this;
}

dir_watcher::~dir_watcher() {
    _file_handle.reset();
}

void dir_watcher::watch(const std::wstring& path_) {
    if ( !is_runging() ) {
        start();
    } else {
        change_state_and_wait(state::sleep);
    }
    _name = path_;
    change_state_and_wait(state::init);
}

void dir_watcher::stop() {
    change_state(state::sleep);
    _file_handle.reset();
}