#pragma once

#include <byte_order.h>
#include <boost/asio.hpp>
#include <boost/optional.hpp>

class nat {
    struct pub_ip_request {
        byte_order::net_uchar_t     version;
        byte_order::net_uchar_t     opcode;
    };
    struct pub_ip_response {
        byte_order::net_uchar_t     version;
        byte_order::net_uchar_t     opcode;
        byte_order::net_ushort_t    command;
        byte_order::net_ulong_t     time;
        byte_order::net_ulong_t     ipv4;
    };

    enum {
        nat_port = 5351
    };

    boost::asio::io_service*        _service;
    std::vector<unsigned short>     _ports;

    boost::optional<boost::asio::ip::address_v4> get_public_ip(boost::asio::ip::udp::endpoint gateway_) {
        boost::asio::ip::udp::socket    socket(*_service);
        socket.open(boost::asio::ip::udp::v4());

        pub_ip_request r = { 0, 0 };
        pub_ip_response resp = {};
        boost::system::error_code ec;
        socket.send_to(boost::asio::buffer(&r, sizeof( r )), gateway_, 0, ec);
        if ( ec ) {
            return boost::optional<boost::asio::ip::address_v4>( );
        }

        boost::asio::ip::udp::endpoint resp_sender;
        socket.receive_from(boost::asio::buffer(&resp, sizeof( resp )), resp_sender, 0, ec);
        if ( ec ) {
            return boost::optional<boost::asio::ip::address_v4>( );
        }

        if ( resp_sender.address() != gateway_.address() ) {
            return boost::optional<boost::asio::ip::address_v4>( );
        }

        if ( resp.opcode != r.opcode + 128 ) {
            return boost::optional<boost::asio::ip::address_v4>( );
        }

        if ( resp.command ) {
            return boost::optional<boost::asio::ip::address_v4>( );
        }

        return  boost::asio::ip::address_v4(resp.ipv4);
    }

public:
    boost::optional<unsigned short> register_port_forward(unsigned short pfwd_);
    void unregister_port_forward(unsigned short pfwd_);
};