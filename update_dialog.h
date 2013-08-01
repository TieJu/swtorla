#pragma once

#include "dialog.h"

#include "resource.h"

#include <Windows.h>
#include <CommCtrl.h>

class update_dialog
: public dialog {

public:
    update_dialog()
        : dialog(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDD_UPDATE_POPUP), nullptr) {
    }

    void info_msg(const std::wstring& str_) {
        ::SetWindowTextW(GetDlgItem(native_handle(), IDC_UPDATE_INFO), str_.c_str());
    }
    void progress(int pos_, int range_from_ = 0, int range_to_ = 100) {
        auto element = GetDlgItem(native_handle(), IDC_UPDATE_PROGRESS);
        ::PostMessageW(element, PBM_SETRANGE32, range_from_, range_to_);
        ::PostMessageW(element, PBM_SETPOS, pos_, 0);
    }
    void unknown_progress(bool unknown_) {
        auto element = GetDlgItem(native_handle(), IDC_UPDATE_PROGRESS);
        if ( unknown_ ) {
            ::SetWindowLongPtrW(element, GWL_STYLE, ::GetWindowLongPtrW(element, GWL_STYLE) | PBS_MARQUEE);
            ::SetWindowPos(element, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
            ::PostMessageW(element, PBM_SETMARQUEE, TRUE, 0);
        } else {
            ::SetWindowLongPtrW(element, GWL_STYLE, ::GetWindowLongPtrW(element, GWL_STYLE) & ~PBS_MARQUEE);
            ::SetWindowPos(element, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        }
    }
    /*
    #ifndef PBM_SETSTATE
    #define PBM_SETSTATE            (WM_USER+16)
    #define PBM_GETSTATE            (WM_USER+17)
    #define PBST_NORMAL             0x0001
    #define PBST_ERROR              0x0002
    #define PBST_PAUSED             0x0003
    #endif
    WPARAM value = 0;
    switch ( e_.level ) {
    case progress_bar::display_state::normal:
    value = PBST_NORMAL;
    break;
    case progress_bar::display_state::error:
    value = PBST_ERROR;
    break;
    case progress_bar::display_state::paused:
    value = PBST_PAUSED;
    break;
    }
    ::PostMessageW(GetDlgItem(_wnd->native_handle(), IDC_UPDATE_PROGRESS), PBM_SETSTATE, value, 0);*/
};