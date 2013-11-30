#pragma once

#include "swtor_log_parser.h"
#include "int_compress.h"
#include "active.h"
#include "socket.h"

class app;

class net_link_server {
    app*        _ci;
    c_socket    _link;

public:
    net_link_server();
    explicit net_link_server(app* ci_);
    net_link_server(net_link_server&& other_);
    ~net_link_server();
    net_link_server& operator=(net_link_server&& other_);

    void start(const std::string& port_);
    void stop( );
    void on_socket_event( SOCKET socket_, unsigned event_, unsigned error_ );
};