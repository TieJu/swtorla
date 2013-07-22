#include "path_finder.h"

#include <boost/scope_exit.hpp>

#include <Shlobj.h>
#include <Shellapi.h>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

std::wstring find_swtor_log_path() {
    std::wstring path;
    
    wchar_t base_loc[MAX_PATH * 2]{};
    if ( SHGetSpecialFolderPath(nullptr, base_loc, CSIDL_PERSONAL, FALSE) ) {
        path = base_loc;
        path += L"\\Star Wars - The Old Republic\\CombatLogs";
    }

    return path;
}

std::wstring find_path_for_lates_log_file(const std::wstring& path_) {
    auto search = path_ + L"\\combat_*.txt";

    WIN32_FIND_DATAW info{};
    auto handle = FindFirstFileW(search.c_str(), &info);

    if ( handle != INVALID_HANDLE_VALUE ) {
        BOOST_SCOPE_EXIT_ALL(= ) {
            FindClose(handle);
        };

        auto last_info = info;

        do {
            BOOST_LOG_TRIVIAL(debug) << L"log file found " << info.cFileName;
            if ( CompareFileTime(&last_info.ftCreationTime, &info.ftCreationTime) < 0 ) {
                last_info = info;
            }
        } while ( FindNextFileW(handle, &info) );

        BOOST_LOG_TRIVIAL(debug) << L"newest log file is " << last_info.cFileName;

        return path_ + L"\\" + last_info.cFileName;
    }
    return std::wstring();
}