#include "program_version.h"

std::wstring get_version_param(LPCVOID block, const wchar_t* entry) {
    UINT vLen;
    DWORD langD;
    BOOL retVal;

    LPVOID retbuf = NULL;

    static wchar_t fileEntry[256];

    ::swprintf_s(fileEntry, L"\\VarFileInfo\\Translation");
    retVal = ::VerQueryValueW(block, fileEntry, &retbuf, &vLen);
    if ( retVal && vLen == 4 ) {
        ::memcpy(&langD, retbuf, 4);
        ::swprintf_s( fileEntry
                    , L"\\StringFileInfo\\%02X%02X%02X%02X\\%s"
                    , ( langD & 0xff00 ) >> 8
                    , langD & 0xff
                    , ( langD & 0xff000000 ) >> 24
                    , ( langD & 0xff0000 ) >> 16
                    , entry);
    } else {
        ::swprintf_s( fileEntry
                    , L"\\StringFileInfo\\%04X04B0\\%s"
                    , GetUserDefaultLangID()
                    , entry);
    }

    if ( ::VerQueryValueW(block, fileEntry, &retbuf, &vLen) ) {
        return reinterpret_cast<const wchar_t*>( retbuf );
    }
    return std::wstring();
}

program_version find_version_info(LPCWSTR resource_name_) {
    program_version info{};

    auto h_version_res = ::FindResourceW(nullptr, resource_name_, RT_VERSION);

    if ( !h_version_res ) {
        return info;
    }

    auto h_loaded_version = ::LoadResource(nullptr, h_version_res);

    if ( !h_loaded_version ) {
        return info;
    }

    auto h_locked_version = ::LockResource(h_loaded_version);

    auto version = get_version_param(h_locked_version, L"ProductVersion");

    swscanf_s(version.c_str(), L"%d.%d.%d.Build%d", &info.major, &info.minor, &info.patch, &info.build);

    UnlockResource(h_loaded_version);
    ::FreeResource(h_loaded_version);

    return info;
}