#pragma once

#include "swtor_log_parser.h"
#include "int_compress.h"
#include "active.h"
#include "net_link_base.h"

#include <boost/optional.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>

class app;

class server_net_link
    : public active<server_net_link>
    , public net_link_base<server_net_link> {
    app*                                            _ci;
    std::unique_ptr<boost::asio::ip::tcp::socket>   _link;

    friend class net_link_base<server_net_link>;
    void on_net_link_command(command command_, const char* data_begin_, const char* data_end_);
    boost::asio::ip::tcp::socket& get_link();
    bool is_link_active();

    friend class active<server_net_link>;
    void run();

public:
    server_net_link(app& app_, boost::asio::ip::tcp::socket* socket_);
    ~server_net_link();
    void send_set_name(string_id id_, const std::wstring& name_);
};