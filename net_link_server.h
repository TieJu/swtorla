#pragma once

#include "swtor_log_parser.h"
#include "int_compress.h"
#include "active.h"

#include <boost/optional.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>

class app;

class net_link_server {
    app*                                            _ci;
    std::unique_ptr<boost::asio::ip::tcp::acceptor> _link;
    std::unique_ptr<boost::asio::ip::tcp::socket>   _target;

public:
    net_link_server();
    explicit net_link_server(app* ci_);
    net_link_server(net_link_server&& other_);
    ~net_link_server();
    net_link_server& operator=(net_link_server&& other_);

    void start(const std::string& port_);
    void stop();
    void operator()();
};