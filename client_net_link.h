#pragma once

#include "swtor_log_parser.h"
#include "int_compress.h"
#include "active.h"
#include "net_link_base.h"

#include <boost/optional.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>

class app;

class client_net_link
    : public active<client_net_link>
    , public net_link_base<client_net_link> {
    app*                                            _ci;
    std::unique_ptr<boost::asio::ip::tcp::socket>   _link;
    std::string                                     _peer;
    std::string                                     _peer_port;

    friend class net_link_base<client_net_link>;
    void on_net_link_command(command command_, const char* data_begin_, const char* data_end_);
    bool init_link(const std::string& peer_, const std::string& port_);
    void shutdown_link();
    boost::asio::ip::tcp::socket& get_link();
    bool is_link_active();

    friend class active<client_net_link>;
    void run();

public:
    client_net_link();
    client_net_link(app& ci_);
    client_net_link(client_net_link&& other_);
    ~client_net_link();
    client_net_link& operator=( client_net_link && other_ );

    void connect(const std::string& name_, const std::string& port_);
    void disconnect();

    void register_at_server(const std::wstring& name_);
};