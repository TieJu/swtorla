#include "app.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

net_link_server::net_link_server()
    : _ci(nullptr) {
}

net_link_server::net_link_server(app* ci_)
    : _ci(ci_) {
}
net_link_server::net_link_server(net_link_server&& other_)
    : net_link_server() {
    *this = std::move(other_);
}

net_link_server::~net_link_server() {
}

net_link_server& net_link_server::operator=( net_link_server && other_ ) {
    _ci = std::move( other_._ci );
    _link = std::move( other_._link );
    return *this;
}

void net_link_server::start( const std::string& port_ ) {
    BOOST_LOG_TRIVIAL( debug ) << L"void net_link_server::start(" << port_ << L");";
    socket_api sapi {};
    _link = c_socket { sapi, AF_INET, SOCK_STREAM, 0 };
    socket_address_inet addr;
    addr.port() = htons( std::stoul( port_ ) );
    _link.bind( addr );
    _ci->register_listen_socket( _link );
    _link.listen();
}

void net_link_server::stop( ) {
    _link.reset();
}

void net_link_server::on_socket_event( SOCKET socket_, unsigned event_, unsigned error_ ) {
    if ( FD_ACCEPT == event_ ) {
        c_socket s;
        while ( ( s = _link.accept() ) ) {
            _ci->new_client( std::move( s ) );
        }
    }
}