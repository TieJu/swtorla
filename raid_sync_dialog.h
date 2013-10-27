#pragma once

#include "dialog.h"
#include "resource.h"

class raid_sync_dialog
    : public dialog_t<raid_sync_dialog> {

protected:
    INT_PTR on_command( WPARAM wParam, LPARAM lParam ) {
        auto id = LOWORD( wParam );
        auto code = HIWORD( wParam );
        switch ( id ) {
        case IDC_RAID_SYNC_MODE:
        case IDC_RAID_SYNC_SERVER_ADDRESS:
            break;
        case IDC_SERVER_SELECT_OK: // TODO: do the selected stuff
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
        auto icon = ::LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_ICON1));
        ::SendMessageW(native_handle(), WM_SETICON, ICON_BIG, (LPARAM)icon);
        ::SendMessageW( native_handle(), WM_SETICON, ICON_SMALL, (LPARAM)icon );

        auto server_mode = ::GetDlgItem( native_handle(), IDC_RAID_SYNC_MODE );
        ::SendMessageW( server_mode, CB_ADDSTRING, 0, (LPARAM)L"Client" );
        ::SendMessageW( server_mode, CB_ADDSTRING, 0, (LPARAM)L"Server - Public IP4" );
        ::SendMessageW( server_mode, CB_ADDSTRING, 0, (LPARAM)L"Server - Public Hash" );
        ::SendMessageW( server_mode, CB_SETCURSEL, 0, 0 );

        MSG msg{};
        while ( GetMessageW(&msg, nullptr, 0, 0) ) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
};