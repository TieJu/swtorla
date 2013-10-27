#pragma once

#include "dialog.h"
#include "resource.h"

class raid_sync_dialog
    : public dialog_t<raid_sync_dialog> {

protected:
    void start_server( int mode_ ) {
    }
    void start_client( const std::wstring& target_ ) {
    }
    std::wstring get_public_ip() {
        return L"<last server>";
    }
    std::wstring get_public_hash() {
        return L"<public ip>";
    }
    std::wstring get_last_server() {
        return L"<public hash>";
    }
    void start_raid_sync() {
        auto mode = SendMessageW( ::GetDlgItem( native_handle(), IDC_RAID_SYNC_MODE ), CB_GETCURSEL, 0, 0 );
        if ( mode == 0 ) {
            start_client( GetWindowTextW( ::GetDlgItem( native_handle(), IDC_RAID_SYNC_SERVER_ADDRESS ) ) );
        } else if ( mode == 1 || mode == 2 ) {
            start_server( mode );
        }
    }

    INT_PTR on_command( WPARAM wParam, LPARAM lParam ) {
        auto id = LOWORD( wParam );
        auto code = HIWORD( wParam );
        switch ( id ) {
        case IDC_RAID_SYNC_MODE:
            if ( code == CBN_SELCHANGE ) {
                auto mode = SendMessageW( ::GetDlgItem( native_handle(), IDC_RAID_SYNC_MODE ), CB_GETCURSEL, 0, 0 );
                std::wstring addr;
                if ( mode == 0 ) {
                    addr = get_last_server();
                } else if ( mode == 1 ) {
                    addr = get_public_ip();
                } else {
                    addr = get_public_hash();
                }
                ::SetWindowTextW( ::GetDlgItem( native_handle(), IDC_RAID_SYNC_SERVER_ADDRESS ), addr.c_str() );
            }
            break;
        case IDC_RAID_SYNC_SERVER_ADDRESS:
            break;
        case IDC_SERVER_SELECT_OK:
            start_raid_sync();
        case IDC_SERVER_SELECT_CANCEL:
            ::DestroyWindow( native_handle() );
            return TRUE;
        }
        return FALSE;
    }
    INT_PTR on_notify(WPARAM wParam, LPARAM lParam) {
        return FALSE;
    }
    friend class dialog_t<raid_sync_dialog>;
    INT_PTR on_window_event(UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch ( uMsg ) {
        case WM_CLOSE:
            ::DestroyWindow( native_handle() );
            return TRUE;
        case WM_DESTROY:
            ::PostQuitMessage(0);
            return TRUE;
        case WM_COMMAND:
            return on_command(wParam, lParam);
        case WM_NOTIFY:
            return on_notify(wParam, lParam);
        case WM_INITDIALOG:
            return TRUE;
        }
        return FALSE;
    }

public:
    raid_sync_dialog()
        : dialog_t<raid_sync_dialog>( GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDD_RAID_SYNC), nullptr ) {
        auto icon = ::LoadIconW( GetModuleHandleW( nullptr ), MAKEINTRESOURCEW( IDI_ICON1 ) );
        ::SendMessageW( native_handle(), WM_SETICON, ICON_BIG, (LPARAM)icon );
        ::SendMessageW( native_handle(), WM_SETICON, ICON_SMALL, (LPARAM)icon );

        auto server_mode = ::GetDlgItem( native_handle(), IDC_RAID_SYNC_MODE );
        ::SendMessageW( server_mode, CB_ADDSTRING, 0, (LPARAM)L"Client" );
        ::SendMessageW( server_mode, CB_ADDSTRING, 0, (LPARAM)L"Server - Public IP4" );
        ::SendMessageW( server_mode, CB_ADDSTRING, 0, (LPARAM)L"Server - Public Hash" );
        ::SendMessageW( server_mode, CB_SETCURSEL, 0, 0 );

        ::SetWindowTextW( ::GetDlgItem( native_handle(), IDC_RAID_SYNC_SERVER_ADDRRESS ), get_last_server().c_str() );

        MSG msg{};
        while ( GetMessageW(&msg, nullptr, 0, 0) ) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
};