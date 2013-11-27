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
    : _ci(ci_)
    , _link(new boost::asio::ip::tcp::acceptor(ci_->get_io_service())) {
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
    _target = std::move( other_._target );
    return *this;
}

void net_link_server::start( const std::string& port_ ) {
    BOOST_LOG_TRIVIAL( debug ) << L"void net_link_server::start(" << port_ <<  L");";
    _link->open( boost::asio::ip::tcp::v4( ) );
    _link->bind( boost::asio::ip::tcp::endpoint( boost::asio::ip::tcp::v4( ), std::stoul( port_ ) ) );
    _link->non_blocking( true );
    _link->listen( );
}

void net_link_server::stop( ) {
    _link->close( );
}

void net_link_server::operator()() {
    if ( !_link ) {
        return;
    }
    if ( !_link->is_open() ) {
        return;
    }

    if ( !_target ) {
        _target.reset( new boost::asio::ip::tcp::socket( _ci->get_io_service() ) );
    }

    boost::system::error_code error;
    _link->accept( *_target, error );
    if ( error ) {
        if ( error != boost::asio::error::would_block && error != boost::asio::error::try_again ) {
            BOOST_LOG_TRIVIAL( error ) << L"...accept failed because, " << error.message( );
        }
    } else {
        _ci->new_client( _target.release( ) );
    }
}