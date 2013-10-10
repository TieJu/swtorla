#pragma once

#include "dialog.h"
#include "resource.h"

class update_info_dialog
    : public dialog {

public:
    update_info_dialog(const std::wstring& text_,bool* do_auto_update_,bool* display_update_info_)
        : dialog(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDD_UPDATE_INFO), nullptr) {
        callback([=](dialog* dlg_, UINT msg_, WPARAM w_param_, LPARAM l_param_) {
            if ( msg_ == WM_CLOSE || msg_ == WM_DESTROY ) {
                ::PostQuitMessage(0);
            } else if ( msg_ == WM_COMMAND ) {
                auto id = LOWORD(w_param_);
                auto code = HIWORD(w_param_);
                if ( code == BN_CLICKED ) {
                    switch ( id ) {
                    case IDC_UPDATE_INFO_RESTART:
                        ::PostQuitMessage(0);
                        break;
                    case IDC_UPDATE_INFO_DO_UPDATES:
                        if ( BST_UNCHECKED == ::SendDlgItemMessageW(dlg_->native_handle(), IDC_UPDATE_INFO_DO_UPDATES, BM_GETCHECK, 0, 0) ) {
                            ::EnableWindow(::GetDlgItem(dlg_->native_handle(), IDC_UPDATE_INFO_SHOW_INFO), FALSE);
                            *do_auto_update_ = false;
                        } else {
                            ::EnableWindow(::GetDlgItem(dlg_->native_handle(), IDC_UPDATE_INFO_SHOW_INFO), TRUE);
                            *do_auto_update_ = true;
                        }
                        break;
                    case IDC_UPDATE_INFO_SHOW_INFO:
                        *display_update_info_ = BST_UNCHECKED == ::SendDlgItemMessageW(dlg_->native_handle(), IDC_UPDATE_INFO_SHOW_INFO, BM_GETCHECK, 0, 0);
                        break;
                    }
                }
            }
            return FALSE;
        });
        auto icon = ::LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_ICON1));
        ::SendMessageW(native_handle(), WM_SETICON, ICON_BIG, (LPARAM)icon);
        ::SendMessageW(native_handle(), WM_SETICON, ICON_SMALL, (LPARAM)icon);

        ::SendDlgItemMessageW(native_handle(), IDC_UPDATE_INFO_DO_UPDATES, BM_SETCHECK, BST_CHECKED, 0);
        ::SendDlgItemMessageW(native_handle(), IDC_UPDATE_INFO_SHOW_INFO, BM_SETCHECK, BST_CHECKED, 0);

        ::SetDlgItemTextW(native_handle(), IDC_UPDATE_INFO_EDIT, text_.c_str());
    }
};