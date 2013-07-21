#include "dir_watcher.h"

#include <boost/scope_exit.hpp>

void dir_watcher::on_added_file(const wchar_t* begin_, const wchar_t* end_) {
    BOOST_LOG_TRIVIAL(debug) << L"added file " << std::wstring(begin_, end_);
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
    const DWORD filter_mask = FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_FILE_NAME;

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
    if ( !ReadDirectoryChangesW(*_file_handle, _buffer[0].data(), _buffer[0].size(), FALSE, filter_mask, nullptr, &state, nullptr) ) {
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
            if ( !ReadDirectoryChangesW(*_file_handle, _buffer[0].data(), _buffer[0].size(), FALSE, filter_mask, nullptr, &state, nullptr) ) {
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

dir_watcher::dir_watcher(const std::wstring& path)
    : _file_handle(INVALID_HANDLE_VALUE)
    , _handler_thread([=]() {
        try {
            thread_entry(path);
        } catch ( const std::runtime_error& e ) {
            BOOST_LOG_TRIVIAL(error) << L"dir observation failed, because " << e.what() << ", ending observer thread";
        }
    }) {
}

dir_watcher::~dir_watcher() {
    //auto copy = _file_handle.release();
    //CloseHandle(copy);
    _file_handle.reset();
    _handler_thread.join();
}