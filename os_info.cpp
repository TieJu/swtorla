#include "os_info.h"

void os_info::find_windows_type() {
    OSVERSIONINFOEXW osvi{};
    BOOL bOsVersionInfoEx;
    DWORD dwType;

    osvi.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );
    bOsVersionInfoEx = GetVersionExW((OSVERSIONINFO*)&osvi);

    if ( bOsVersionInfoEx == FALSE ) {
        return;
    }

    if ( VER_PLATFORM_WIN32_NT == osvi.dwPlatformId && osvi.dwMajorVersion > 4 ) {
        if ( osvi.dwMajorVersion > 6 ) {
            _type = windows_type::later_than_8_1;
        } else if ( osvi.dwMajorVersion == 6 ) {
            if ( osvi.dwMinorVersion == 0 ) {
                if ( osvi.wProductType == VER_NT_WORKSTATION ) {
                    _type = windows_type::vista;
                } else {
                    _type = windows_type::_2k8;
                }
            } else if ( osvi.dwMinorVersion == 1 ) {
                if ( osvi.wProductType == VER_NT_WORKSTATION ) {
                    _type = windows_type::_7;
                } else {
                    _type = windows_type::_2k8r2;
                }
            } else if ( osvi.dwMinorVersion == 2 ) {
                if ( osvi.wProductType == VER_NT_WORKSTATION ) {
                    _type = windows_type::_8;
                    // additional steps are needed to detect win 8.1
                } else {
                    _type = windows_type::_2k12;
                    // additional steps are needed to detect win _2k12r2
                }
            } else if ( osvi.dwMinorVersion > 2 ) {
                _type = windows_type::later_than_8_1;
            }
        } else if ( osvi.dwMajorVersion == 5 && ( osvi.dwMinorVersion == 2 || osvi.dwMinorVersion == 1 ) ) {
            _type = windows_type::xp;
        }
    }
}

void os_info::check_64_bit_os() {
#if defined(_WIN64)
    _is_64bit = true;
#else
    typedef BOOL(WINAPI *LPFN_ISWOW64PROCESS) ( HANDLE, PBOOL );

    _is_64bit = false;

    auto h_kernel = ::GetModuleHandleW(L"kernel32");
    auto pf_IsWow64Process = reinterpret_cast<LPFN_ISWOW64PROCESS>(::GetProcAddress(h_kernel, "IsWow64Process"));

    if ( !pf_IsWow64Process ) {
        return;
    }

    BOOL result;
    if ( pf_IsWow64Process(::GetCurrentProcess(), &result) ) {
        _is_64bit = result == TRUE;
    }
#endif
}
