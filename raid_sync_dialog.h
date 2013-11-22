#pragma once

#include "dialog.h"
#include "resource.h"

class app;

class raid_sync_dialog
    : public dialog_t<raid_sync_dialog> {
        app&    _app;
protected:
    void update_config();
    bool start_server_at_port( unsigned long port_ );
    bool register_at_hash_server( const std::wstring& hash_, unsigned long port_ );
    bool start_server( int mode_ );
    bool connect_to_server( const std::wstring& name_, const std::wstring& port_ );
    pplx::task<std::tuple<std::wstring, std::wstring>> get_ip_and_port_from_hash_task_job( const std::wstring& hash_ );
    std::future<std::tuple<std::wstring, std::wstring>> get_ip_and_port_from_hash_task( const std::wstring& hash_ );
    std::tuple<std::wstring, std::wstring> get_ip_and_port_from_hash( const std::wstring& hash_ );
    bool start_client( std::wstring ip_, std::wstring port_ );
    std::tuple<std::wstring, std::wstring> get_public_ip();
    std::tuple<std::wstring, std::wstring> get_public_hash();
    std::tuple<std::wstring, std::wstring> get_last_server();
    bool start_raid_sync();

    INT_PTR on_command( WPARAM wParam, LPARAM lParam );
    INT_PTR on_notify( WPARAM wParam, LPARAM lParam );
    friend class dialog_t<raid_sync_dialog>;
    INT_PTR on_window_event( UINT uMsg, WPARAM wParam, LPARAM lParam );

public:
    raid_sync_dialog( app& app_ );
};