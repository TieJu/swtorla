#pragma once

#include <functional>

#include "swtor_log_parser.h"
#include "int_compress.h"
#include "active.h"
#include "net_link_base.h"
#include "socket.h"

class app;

class client_net_link
    : public net_link_base<client_net_link> {
    enum {
        buffer_size = 1024
    };
    socket_api                                  _sapi;
    app*                                        _ci;
    c_socket                                    _link;
    std::array<char, buffer_size>               _buffer;
    std::array<char, MAXGETHOSTSTRUCT>          _dns_buff;
    std::function<void( unsigned error_code_ )> _on_connect;

    friend class net_link_base<client_net_link>;
    void on_net_link_command( command command_, const char* data_begin_, const char* data_end_ );
    unsigned connect_link( const socket_address_inet& target_ );
    void shutdown_link();
    c_socket& get_link( );
    bool is_link_active();

    friend class active<client_net_link>;
    void run();

public:
    client_net_link();
    client_net_link(app* ci_);
    client_net_link(client_net_link&& other_);
    ~client_net_link();
    client_net_link& operator=( client_net_link && other_ );

    void connect( const std::string& peer_, const std::string& port_, std::function<void( unsigned error_code_ )> on_connect_ );
    void disconnect();

    void register_at_server(const std::wstring& name_);

    void on_socket_event( SOCKET socket_, unsigned event_, unsigned error_ );
};