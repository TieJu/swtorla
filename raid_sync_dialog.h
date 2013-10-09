#pragma once

#include "dialog.h"
#include "resource.h"

class raid_sync_dialog
    : public dialog_t<raid_sync_dialog> {

protected:
    INT_PTR on_command(WPARAM wParam, LPARAM lParam) {
        return FALSE;
    }
    INT_PTR on_notify(WPARAM wParam, LPARAM lParam) {
        return FALSE;
    }
    friend class dialog_t<raid_sync_dialog>;
    INT_PTR on_window_event(UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch ( uMsg ) {
        case WM_CLOSE:
        case WM_DESTROY:
            ::PostQuitMessage(0);
            return FALSE;
        case WM_COMMAND: return on_command(wParam, lParam);
        case WM_NOTIFY: return on_notify(wParam, lParam);
        case WM_INITDIALOG: return TRUE;
        }
        return FALSE;
    }

public:
    raid_sync_dialog()
        : dialog_t<raid_sync_dialog>( GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDD_RAID_SYNC), nullptr ) {
        auto icon = ::LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_ICON1));
        ::SendMessageW(native_handle(), WM_SETICON, ICON_BIG, (LPARAM)icon);
        ::SendMessageW(native_handle(), WM_SETICON, ICON_SMALL, (LPARAM)icon);

        MSG msg{};
        while ( GetMessageW(&msg, nullptr, 0, 0) ) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
};