#include "app.h"
#include "server_net_link.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

void server_net_link::on_net_link_command(command command_, const char* data_begin_, const char* data_end_) {
    switch ( command_ ) {
    case command::client_register:
        _ci->on_client_register(this, std::wstring(reinterpret_cast<const wchar_t*>( data_begin_ ), reinterpret_cast<const wchar_t*>( data_end_ )));
        break;
    case command::string_lookup:
        _ci->on_string_lookup(this, *reinterpret_cast<const string_id*>( data_begin_ ));
        break;
    case command::string_info:
        _ci->on_string_info(this
                           ,*reinterpret_cast<const string_id*>( data_begin_ )
                           ,std::wstring(reinterpret_cast<const wchar_t*>( data_begin_ + sizeof( string_id ) ), reinterpret_cast<const wchar_t*>( data_end_ )));
        break;
    case command::combat_event:
        _ci->on_combat_event(this, std::get<0>( uncompress(data_begin_, 0) ));
        break;
    default:
        throw std::runtime_error("unknown net command recived");
    }
}

c_socket& server_net_link::get_link() {
    return _link;
}

bool server_net_link::is_link_active() {
    return true;
}

server_net_link::server_net_link( app* app_, c_socket socket_ )
    : _ci(app_)
    , _link(std::move(socket_)) {
}

server_net_link::~server_net_link() {
}

void server_net_link::send_set_name( string_id id_, const std::wstring& name_ ) {
    BOOST_LOG_TRIVIAL( debug ) << L"void server_net_link::send_set_name(" << id_ << L", " << name_ << L");";
    if ( is_link_active() ) {
        auto header = gen_packet_header( command::server_set_name, sizeof(id_)+name_.length() * sizeof( wchar_t ) );
        _link.send( header );
        _link.send( id_ );
        _link.send( name_.data(), name_.length() * sizeof( wchar_t ) );
    }
}
/*
bool server_net_link::operator()( ) {
    boost::system::error_code error;
    for ( ;; ) {
        auto result = _link->read_some( boost::asio::buffer( _buffer ), error );
        if ( result > 0 ) {
            on_net_packet( _buffer.data( ), result );
            continue;
        }

        if ( error ) {
            if ( error != boost::asio::error::would_block ) {
                return false;
            } else {
                break;
            }
        }
    }
    return true;
}

*/
void server_net_link::on_socket_event( SOCKET socket_, unsigned event_, unsigned error_ ) {
    if ( _link.get() != socket_ ) {
        return;
    }
    if ( FD_READ == event_ ) {
        if ( WSAENETDOWN == error_ ) {
            ::MessageBoxA( nullptr, "Lost connection to server", "Connection error", 0 );
            _link.reset( );
        } else {
            int result;
            while ( ( result = ::recv( socket_, _buffer.data( ), _buffer.size( ), 0 ) ) > 0 ) {
                on_net_packet( _buffer.data( ), result );
            }
        }
    } else if ( FD_WRITE == event_ ) {
        if ( WSAENETDOWN == error_ ) {
            ::MessageBoxA( nullptr, "Lost connection to server", "Connection error", 0 );
            _link.reset( );
        } else {
        }
    } else if ( FD_CONNECT == event_ ) {
        //_on_connect( error_ );
        //_on_connect = decltype( _on_connect ){};
        if ( error_ != NOERROR ) {
            _link.reset( );
        }
    } else if ( FD_CLOSE == event_ ) {
        if ( WSAENETDOWN == error_ ) {
            ::MessageBoxA( nullptr, "Lost connection to server", "Connection error", 0 );
            _link.reset( );
        } else if ( WSAECONNRESET == error_ ) {
            ::MessageBoxA( nullptr, "Server has closed the connection", "Connection error", 0 );
            _link.reset( );
        } else if ( WSAECONNABORTED == error_ ) {
            ::MessageBoxA( nullptr, "Server timed out", "Connection error", 0 );
            _link.reset( );
        } else {
            _link.reset( );
        }
    }
}
