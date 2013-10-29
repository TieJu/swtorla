#pragma once

#include "dialog.h"
#include "resource.h"

class raid_sync_dialog
    : public dialog_t<raid_sync_dialog> {
        boost::property_tree::wptree&   _config;
protected:
    void update_config() {
        auto mode = SendMessageW( ::GetDlgItem( native_handle(), IDC_RAID_SYNC_MODE ), CB_GETCURSEL, 0, 0 );
        auto addr = GetWindowTextW( ::GetDlgItem( native_handle(), IDC_RAID_SYNC_SERVER_ADDRESS ) );
        auto port = GetWindowTextW( ::GetDlgItem( native_handle(), IDC_RAID_SYNC_SERVER_ADDRESS2 ) );

        switch ( mode ) {
        default:
        case 0:
            _config.put( L"client.server.ip", addr );
            _config.put( L"client.server.port", port );
            break;
        case 1:
            _config.put( L"server.public.ip", addr );
            _config.put( L"server.public.port", port );
            break;
        case 2:
            _config.put( L"server.hash.hash", addr );
            _config.put( L"server.hash.port", port );
            break;
        }
    }
    void start_server( int mode_ ) {
    }
    void start_client( const std::wstring& target_ ) {
    }
    std::tuple<std::wstring, std::wstring> get_public_ip() {
        auto locals = get_local_ip_addresses();
        return std::make_tuple(_config.get<std::wstring>(L"server.public.ip", locals.empty() ? L"" : locals[0]), _config.get<std::wstring>(L"server.public.port", L"67890"));
    }
    std::tuple<std::wstring, std::wstring> get_public_hash() {
        return std::make_tuple( _config.get<std::wstring>( L"server.hash.hash", L"" ), _config.get<std::wstring>( L"server.hash.port", L"67890" ) );
    }
    std::tuple<std::wstring, std::wstring> get_last_server() {
        return std::make_tuple( _config.get<std::wstring>( L"client.server.ip", L"" ), _config.get<std::wstring>( L"client.server.port", L"67890" ) );
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
                std::wstring addr, port;
                if ( mode == 0 ) {
                    std::tie(addr, port) = get_last_server();
                } else if ( mode == 1 ) {
                    std::tie(addr, port) = get_public_ip();
                } else {
                    std::tie(addr, port) = get_public_hash();
                }
                ::SetWindowTextW( ::GetDlgItem( native_handle(), IDC_RAID_SYNC_SERVER_ADDRESS ), addr.c_str() );
                ::SetWindowTextW( ::GetDlgItem( native_handle(), IDC_RAID_SYNC_SERVER_ADDRESS2 ), port.c_str() );
            }
            break;
        case IDC_RAID_SYNC_SERVER_ADDRESS:
        case IDC_RAID_SYNC_SERVER_ADDRESS2:
            break;
        case IDC_SERVER_SELECT_OK:
            update_config();
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
    raid_sync_dialog( boost::property_tree::wptree& cfg_)
        : dialog_t<raid_sync_dialog>( GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDD_RAID_SYNC), nullptr )
        , _config(cfg_) {
        auto icon = ::LoadIconW( GetModuleHandleW( nullptr ), MAKEINTRESOURCEW( IDI_ICON1 ) );
        ::SendMessageW( native_handle(), WM_SETICON, ICON_BIG, (LPARAM)icon );
        ::SendMessageW( native_handle(), WM_SETICON, ICON_SMALL, (LPARAM)icon );

        auto server_mode = ::GetDlgItem( native_handle(), IDC_RAID_SYNC_MODE );
        ::SendMessageW( server_mode, CB_ADDSTRING, 0, (LPARAM)L"Client" );
        ::SendMessageW( server_mode, CB_ADDSTRING, 0, (LPARAM)L"Server - Public IP4" );
        if ( _config.get<bool>( L"server.enable_hash", false ) ) {
            ::SendMessageW( server_mode, CB_ADDSTRING, 0, (LPARAM)L"Server - Public Hash" );
        }
        ::SendMessageW( server_mode, CB_SETCURSEL, 0, 0 );

        auto v = get_last_server();
        ::SetWindowTextW( ::GetDlgItem( native_handle(), IDC_RAID_SYNC_SERVER_ADDRESS ), std::get<0>( v ).c_str() );
        ::SetWindowTextW( ::GetDlgItem( native_handle(), IDC_RAID_SYNC_SERVER_ADDRESS2 ), std::get<1>( v ).c_str() );

        MSG msg{};
        while ( GetMessageW(&msg, nullptr, 0, 0) ) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
};