#include "app.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

void client_net_link::on_net_link_command(command command_, const char* data_begin_, const char* data_end_) {
    switch ( command_ ) {
    case command::string_lookup:
        _ci->on_string_lookup(this, *reinterpret_cast<const string_id*>( data_begin_ ));
        break;
    case command::string_info:
        _ci->on_string_info(this
                           ,*reinterpret_cast<const string_id*>( data_begin_ )
                           ,std::wstring(reinterpret_cast<const wchar_t*>( data_begin_ + sizeof( string_id ) ), reinterpret_cast<const wchar_t*>( data_end_ )));
        break;
    case command::combat_event: {
        _ci->on_combat_event( this, wrap_combat_log_entry_compressed( data_begin_, data_end_ ) );

        }
        break;
    case command::server_set_name:
        _ci->on_set_name(this
                        ,*reinterpret_cast<const string_id*>( data_begin_ )
                        ,std::wstring(reinterpret_cast<const wchar_t*>( data_begin_ + sizeof( string_id ) ), reinterpret_cast<const wchar_t*>( data_end_ )));
        break;
    default:
        throw std::runtime_error("unknown net command recived");
    }
}

unsigned client_net_link::connect_link( const socket_address_inet& target_ ) {
    _link = c_socket { _sapi, AF_INET, SOCK_STREAM, 0 };
    _ci->register_server_link_socket( _link );
    _link.connect( target_ );
    return 0;
}
void client_net_link::shutdown_link( ) {
    BOOST_LOG_TRIVIAL( debug ) << L"void client_net_link::shutdown_link();";
    if ( !_link ) {
        return;
    }

    _link.reset();
    _ci->on_disconnected_from_server(this);
}

c_socket& client_net_link::get_link() {
    return _link;
}

bool client_net_link::is_link_active() {
    return bool( _link );
}

client_net_link::client_net_link()
    : _ci(nullptr) {
}

client_net_link::client_net_link(app* ci_)
    : _ci(ci_) {
}

client_net_link::client_net_link(client_net_link && other_)
    : client_net_link() {
    *this = std::move(other_);
}

client_net_link::~client_net_link() {
}

client_net_link& client_net_link::operator=( client_net_link && other_ ) {
    net_link_base<client_net_link>::operator=( std::move(other_) );
    _ci = other_._ci;
    other_._ci = nullptr;
    _link = std::move( other_._link );
    _buffer = std::move( other_._buffer );
    return *this;
}

void client_net_link::connect( const std::string& peer_, const std::string& port_, std::function<void( unsigned error_code_ )> on_connect_ ) {
    _on_connect = on_connect_;
    auto addr = _sapi.inet_address( peer_ );
    if ( addr.is_none( ) ) {
        _ci->get_host_by_name( peer_, _dns_buff.data( ), _dns_buff.size( ), [=]( unsigned error_code_, unsigned length_ ) {
            if ( error_code_ == ERROR_SUCCESS ) {
                unsigned last_error = NOERROR;
                auto lookup = reinterpret_cast<const hostent*>( _dns_buff.data( ) );
                auto count = lookup->h_length / sizeof( u_long );
                socket_address_inet ip;
                ip.ip( ) = *reinterpret_cast<u_long*>( lookup->h_addr_list[0] );
                ip.port() = htons( std::stoul( port_ ) );
                connect_link( ip );
            } else {
                on_connect_( error_code_ );
            }
        } );
    } else {
        addr.port( ) = htons( std::stoul( port_ ) );
        connect_link( addr );
    }
}

void client_net_link::disconnect() {
    shutdown_link();
}

void client_net_link::register_at_server( const std::wstring& name_ ) {
    BOOST_LOG_TRIVIAL( debug ) << L"void client_net_link::register_at_server(" << name_ << L");";
    if ( is_link_active() ) {
        auto header = gen_packet_header(command::client_register, sizeof(wchar_t)* name_.length());
        _link.send( header );
        _link.send( name_.data(), header.content_length );
    }
}

void client_net_link::on_socket_event( SOCKET socket_, unsigned event_, unsigned error_ ) {
    if ( FD_READ == event_ ) {
        if ( WSAENETDOWN == error_ ) {
            ::MessageBoxA( nullptr, "Lost connection to server", "Connection error", 0 );
            _link.reset();
        } else {
            int result;
            while ( ( result = ::recv( socket_, _buffer.data(), _buffer.size(), 0 ) ) > 0 ) {
                on_net_packet( _buffer.data(), result );
            }
        }
    } else if ( FD_WRITE == event_ ) {
        if ( WSAENETDOWN == error_ ) {
            ::MessageBoxA( nullptr, "Lost connection to server", "Connection error", 0 );
            _link.reset();
        } else {
        }
    } else if ( FD_CONNECT == event_ ) {
        _on_connect( error_ );
        _on_connect = decltype( _on_connect ){};
        if ( error_ != NOERROR ) {
            _link.reset();
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
            _link.reset();
        }
    }
}