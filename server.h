#pragma once

#include "net_protocol.h"
#include "active.h"

class server
: public active<server>
{
    boost::asio::io_service*        _service;
    std::unique_ptr<net_protocol>   _server_port;
    std::wstring                    _hash;
    boost::asio::ip::tcp::endpoint  _public;

    boost::asio::ip::tcp::endpoint  _server_bind_point;
    bool                            _register_at_global_server;

    boost::asio::ip::tcp::endpoint register_port_forward(boost::asio::ip::tcp::endpoint port_);
    void unregister_prot_forward(boost::asio::ip::tcp::endpoint pub_,boost::asio::ip::tcp::endpoint loc_);
    void get_link_hash();
    void erase_link_hash();

protected:
    friend class active<server>;
    void run() {
        state rs = state::sleep;

        for ( ;; ) {
            if ( rs == state::sleep || rs == state::shutdown ) {
                if ( _server_port ) {
                    if ( !_register_at_global_server && !_hash.empty() ) {
                        erase_link_hash();
                    }
                    unregister_prot_forward(_public, _server_bind_point);
                    _server_port.reset();
                }
            }

            if ( rs == state::shutdown ) {
                break;
            }

            rs = wait(rs);

            if ( rs == state::init ) {
                _public = register_port_forward(_server_bind_point);

                _server_port.reset(new net_protocol(*_service));
                _server_port->listen(_server_bind_point);

                if ( _register_at_global_server ) {
                    get_link_hash();
                }
            }

            boost::asio::ip::tcp::socket cl_port(*_service);
            while ( rs == state::run ) {
                if ( _server_port->accpet(cl_port) ) {
                    // new client
                }

                rs = wait_for(rs, std::chrono::milliseconds(25));
            }
        }
    }

public:
    server() : _service(nullptr) {}
    server(boost::asio::io_service& service_) : _service(&service_) {}

    void start(boost::asio::ip::tcp::endpoint port_, bool register_at_global_server_) {
        stop(!register_at_global_server_);

        _server_bind_point = port_;
        _register_at_global_server = register_at_global_server_;
        change_state(state::init);
    }

    std::wstring hash() const {
        return _hash;
    }

    void stop(bool erase_hash_) {
        _register_at_global_server = !erase_hash_;
        change_state(state::sleep);
    }
};