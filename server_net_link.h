#pragma once

#include "swtor_log_parser.h"
#include "int_compress.h"
#include "active.h"
#include "net_link_base.h"
#include "socket.h"

#include <boost/optional.hpp>

class app;

class server_net_link
    : public net_link_base<server_net_link> {
    enum {
        buffer_size = 1024
    };
    app*                            _ci;
    c_socket                        _link;
    std::array<char, buffer_size>   _buffer;

    friend class net_link_base<server_net_link>;
    void on_net_link_command(command command_, const char* data_begin_, const char* data_end_);
    c_socket& get_link();
    bool is_link_active();

public:
    server_net_link(app* app_, c_socket socket_);
    ~server_net_link();
    void send_set_name( string_id id_, const std::wstring& name_ );
    void on_socket_event( SOCKET socket_, unsigned event_, unsigned error_ );
};