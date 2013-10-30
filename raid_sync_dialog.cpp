#include "app.h"
#include "raid_sync_dialog.h"

#include <http_client.h>


void raid_sync_dialog::update_config() {
    auto mode = SendMessageW( ::GetDlgItem( native_handle(), IDC_RAID_SYNC_MODE ), CB_GETCURSEL, 0, 0 );
    auto addr = GetWindowTextW( ::GetDlgItem( native_handle(), IDC_RAID_SYNC_SERVER_ADDRESS ) );
    auto port = GetWindowTextW( ::GetDlgItem( native_handle(), IDC_RAID_SYNC_SERVER_ADDRESS2 ) );

    switch ( mode ) {
    default:
    case 0:
        _app.get_config().put( L"client.server.ip", addr );
        _app.get_config().put( L"client.server.port", port );
        break;
    case 1:
        _app.get_config().put( L"server.public.ip", addr );
        _app.get_config().put( L"server.public.port", port );
        break;
    case 2:
        _app.get_config().put( L"server.hash.hash", addr );
        _app.get_config().put( L"server.hash.port", port );
        break;
    }
}

bool raid_sync_dialog::start_server_at_port( unsigned long port_ ) {
    ::MessageBoxW( nullptr, L"Server mode not implemented yet", L"Missing feature", MB_OK | MB_ICONSTOP );
    return false;
}

bool raid_sync_dialog::register_at_hash_server( const std::wstring& hash_, unsigned long port_ ) {
    ::MessageBoxW( nullptr, L"Hash lookup not supported yet", L"Missing feature", MB_OK | MB_ICONSTOP );
    return false;
}

bool raid_sync_dialog::start_server( int mode_ ) {
    auto port = GetWindowTextW( ::GetDlgItem( native_handle(), IDC_RAID_SYNC_SERVER_ADDRESS2 ) );
    auto port_num = std::stoul( port );
    if ( port.empty() ) {
        port_num = 67890;
    }
    if ( mode_ == 2 ) {
        auto hash = GetWindowTextW( ::GetDlgItem( native_handle(), IDC_RAID_SYNC_SERVER_ADDRESS ) );
        if ( !register_at_hash_server( hash, port_num ) ) {
            // register_at_hash_server will report the error
            return false;
        }
    }

    return start_server_at_port( port_num );
}

bool raid_sync_dialog::connect_to_server( const std::wstring& name_, const std::wstring& port_ ) {
    auto state = _app.connect_to_server( name_, port_ );
    update_dialog dlg;
    dlg.caption( L"...connecting..." );
    dlg.info_msg( L"...connecting to server..." );
    dlg.unknown_progress( true );
    dlg.peek_until( [=, &state]() { return state.wait_for( std::chrono::milliseconds { 100 } ) == std::future_status::ready; } );
    return state.get();
}

std::tuple<std::wstring, std::wstring> raid_sync_dialog::get_ip_and_port_from_hash_task_job( const std::wstring& hash_ ) {
    using namespace web::http;

    auto at = hash_.find( L'@' );
    if ( at == std::wstring::npos ) {
        ::MessageBoxW( nullptr, L"Invalid hash value, lookup failed", L"Hash error", MB_OK | MB_ICONSTOP );
        return std::make_tuple( L"", L"" );
    }

    auto hash = hash_.substr( 1, at - 1 );
    auto server_uri = L"http://" + hash_.substr( at + 1 );
    auto dash = server_uri.find( L"/" );
    std::wstring server, path;
    if ( dash ) {
        server = server_uri.substr( 0, dash );
        path = server_uri.substr( dash + 1 );
    } else {
        server = server_uri;
    }

    web::http::client::http_client client { server };
    uri_builder uri { path };
    uri.append_path( L"hash.php" );
    uri.append_query( L"m", L"lookup" );
    uri.append_query( L"h", hash );

    auto qs = uri.to_string();
    BOOST_LOG_TRIVIAL( debug ) << L"...sending request " << qs << L" to server " << server << L"...";

    return client.request( methods::GET, qs ).then( [=]( http_response result_ ) {
        BOOST_LOG_TRIVIAL( debug ) << L"...server " << server << L" returned " << result_.status_code() << L"...";
        if ( result_.status_code() == status_codes::OK ) {
            return result_.extract_json();
        } else {
            return pplx::task_from_result( web::json::value() );
        }
    } ).then( [=]( pplx::task<web::json::value> json_ ) {
        auto& set = json_.get();
        
        auto ip = set[L"ip"].to_string();
        auto port = set[L"port"].to_string();

        if ( ip == L"null" ) {
            ip.clear();
        }
        if ( port == L"null" ) {
            port.clear();
        }
        
        return std::make_tuple( ip, port );
    } ).get();
}

std::future<std::tuple<std::wstring, std::wstring>> raid_sync_dialog::get_ip_and_port_from_hash_task( const std::wstring& hash_ ) {
    return std::async( std::launch::async, [=]() {
        try {
            return get_ip_and_port_from_hash_task_job( hash_ );
        } catch ( const std::exception& e_ ) {
            BOOST_LOG_TRIVIAL( debug ) << L"...error while looking up hash value " << hash_ << L", " << e_.what();
            return std::make_tuple( std::wstring { L"" }, std::wstring { L"" } );
        }
    } );
}

std::tuple<std::wstring, std::wstring> raid_sync_dialog::get_ip_and_port_from_hash( const std::wstring& hash_ ) {
    auto state = get_ip_and_port_from_hash_task( hash_ );
    update_dialog dlg;
    dlg.caption( L"...translating hash to ip..." );
    dlg.info_msg( L"...waiting on hash server..." );
    dlg.unknown_progress( true );
    dlg.peek_until( [=, &state]() { return state.wait_for( std::chrono::milliseconds { 100 } ) == std::future_status::ready; } );
    auto value = state.get();
    return value;
}

bool raid_sync_dialog::start_client( std::wstring ip_, std::wstring port_ ) {
    if ( ip_.empty() ) {
        ::MessageBoxW( nullptr, L"You need to specify a server address", L"Missing server adderss", MB_OK | MB_ICONSTOP );
        return FALSE;
    }

    // hash format from hash server
    // '#'<hash>'@'<server>
    if ( ip_[0] == '#' ) {
        std::tie( ip_, port_ ) = get_ip_and_port_from_hash( ip_ );
        if ( ip_.empty() ) {
            ::MessageBoxW( nullptr, L"Lookup of hash value failed, see log file for details", L"Hash lookup failed", MB_OK | MB_ICONSTOP );
            return FALSE;
        }
    }

    if ( port_.empty() ) {
        port_ = L"67890";
    }

    return connect_to_server( ip_, port_ );
}

std::tuple<std::wstring, std::wstring> raid_sync_dialog::get_public_ip() {
    auto locals = get_local_ip_addresses();
    return std::make_tuple( _app.get_config().get<std::wstring>( L"server.public.ip", locals.empty() ? L"" : locals[0] ), _app.get_config().get<std::wstring>( L"server.public.port", L"67890" ) );
}

std::tuple<std::wstring, std::wstring> raid_sync_dialog::get_public_hash() {
    return std::make_tuple( _app.get_config().get<std::wstring>( L"server.hash.hash", L"" ), _app.get_config().get<std::wstring>( L"server.hash.port", L"67890" ) );
}

std::tuple<std::wstring, std::wstring> raid_sync_dialog::get_last_server() {
    return std::make_tuple( _app.get_config().get<std::wstring>( L"client.server.ip", L"" ), _app.get_config().get<std::wstring>( L"client.server.port", L"67890" ) );
}

bool raid_sync_dialog::start_raid_sync() {
    auto mode = SendMessageW( ::GetDlgItem( native_handle(), IDC_RAID_SYNC_MODE ), CB_GETCURSEL, 0, 0 );
    if ( mode == 0 ) {
        return start_client( GetWindowTextW( ::GetDlgItem( native_handle(), IDC_RAID_SYNC_SERVER_ADDRESS ) ), GetWindowTextW( ::GetDlgItem( native_handle(), IDC_RAID_SYNC_SERVER_ADDRESS2 ) ) );
    } else if ( mode == 1 || mode == 2 ) {
        return start_server( mode );
    }
    return false;
}

INT_PTR raid_sync_dialog::on_command( WPARAM wParam, LPARAM lParam ) {
    auto id = LOWORD( wParam );
    auto code = HIWORD( wParam );
    switch ( id ) {
    case IDC_RAID_SYNC_MODE:
        if ( code == CBN_SELCHANGE ) {
            auto mode = SendMessageW( ::GetDlgItem( native_handle(), IDC_RAID_SYNC_MODE ), CB_GETCURSEL, 0, 0 );
            std::wstring addr, port;
            if ( mode == 0 ) {
                std::tie( addr, port ) = get_last_server();
            } else if ( mode == 1 ) {
                std::tie( addr, port ) = get_public_ip();
            } else {
                std::tie( addr, port ) = get_public_hash();
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
        if ( !start_raid_sync() ) {
            return TRUE;
        }
    case IDC_SERVER_SELECT_CANCEL:
        ::DestroyWindow( native_handle() );
        return TRUE;
    }
    return FALSE;
}

INT_PTR raid_sync_dialog::on_notify( WPARAM wParam, LPARAM lParam ) {
    return FALSE;
}

INT_PTR raid_sync_dialog::on_window_event( UINT uMsg, WPARAM wParam, LPARAM lParam ) {
    switch ( uMsg ) {
    case WM_CLOSE:
        ::DestroyWindow( native_handle() );
        return TRUE;
    case WM_DESTROY:
        ::PostQuitMessage( 0 );
        return TRUE;
    case WM_COMMAND:
        return on_command( wParam, lParam );
    case WM_NOTIFY:
        return on_notify( wParam, lParam );
    case WM_INITDIALOG:
        return TRUE;
    }
    return FALSE;
}

raid_sync_dialog::raid_sync_dialog( app& app_ )
: dialog_t<raid_sync_dialog>( GetModuleHandleW( nullptr ), MAKEINTRESOURCEW( IDD_RAID_SYNC ), nullptr )
, _app( app_ ) {
    auto icon = ::LoadIconW( GetModuleHandleW( nullptr ), MAKEINTRESOURCEW( IDI_ICON1 ) );
    ::SendMessageW( native_handle(), WM_SETICON, ICON_BIG, (LPARAM)icon );
    ::SendMessageW( native_handle(), WM_SETICON, ICON_SMALL, (LPARAM)icon );

    auto server_mode = ::GetDlgItem( native_handle(), IDC_RAID_SYNC_MODE );
    ::SendMessageW( server_mode, CB_ADDSTRING, 0, (LPARAM)L"Client" );
    ::SendMessageW( server_mode, CB_ADDSTRING, 0, (LPARAM)L"Server - Public IP4" );
    ::SendMessageW( server_mode, CB_ADDSTRING, 0, (LPARAM)L"Server - Public Hash" );
    ::SendMessageW( server_mode, CB_SETCURSEL, 0, 0 );

    auto v = get_last_server();
    ::SetWindowTextW( ::GetDlgItem( native_handle(), IDC_RAID_SYNC_SERVER_ADDRESS ), std::get<0>( v ).c_str() );
    ::SetWindowTextW( ::GetDlgItem( native_handle(), IDC_RAID_SYNC_SERVER_ADDRESS2 ), std::get<1>( v ).c_str() );

    MSG msg {};
    while ( GetMessageW( &msg, nullptr, 0, 0 ) ) {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }
}