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

bool client_net_link::init_link( const std::string& peer_, const std::string& port_ ) {
    BOOST_LOG_TRIVIAL( debug ) << L"void client_net_link::shutdown_link(" << peer_ << L", " << port_ << L");";
    BOOST_LOG_TRIVIAL(debug) << L"looking up remote server address for " << peer_ << L" " << port_;
    boost::asio::ip::tcp::resolver tcp_lookup(_ci->get_io_service());
    boost::asio::ip::tcp::resolver::query remote_query(peer_, port_);
    auto list = tcp_lookup.resolve(remote_query);

    BOOST_LOG_TRIVIAL(debug) << L"connecting to remote server";
    _link.reset(new boost::asio::ip::tcp::socket(_ci->get_io_service()));
    boost::system::error_code error;
    boost::asio::ip::tcp::resolver::iterator lend;
    for ( ; lend != list; ++list ) {
        if ( !_link->connect( *list, error ) ) {
            BOOST_LOG_TRIVIAL(debug) << L"connected to " << list->endpoint();
            break;
        }
    }

    if ( error ) {
        BOOST_LOG_TRIVIAL(error) << L"unable to connect to remote server";
        _link.reset();
        return false;
    }

    _link->non_blocking(true);
    _ci->on_connected_to_server(this);
    return true;
}

void client_net_link::shutdown_link( ) {
    BOOST_LOG_TRIVIAL( debug ) << L"void client_net_link::shutdown_link();";
    if ( !_link ) {
        return;
    }

    _link->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
    _link->close();
    _link.reset();
    _ci->on_disconnected_from_server(this);
}

boost::asio::ip::tcp::socket& client_net_link::get_link() {
    return *_link;
}

bool client_net_link::is_link_active() {
    return _link != nullptr;
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

void client_net_link::connect(const std::string& name_, const std::string& port_) {
    init_link( name_, port_ );
}

void client_net_link::disconnect() {
    shutdown_link();
}

void client_net_link::register_at_server( const std::wstring& name_ ) {
    BOOST_LOG_TRIVIAL( debug ) << L"void client_net_link::register_at_server(" << name_ << L");";
    if ( is_link_active() ) {
        auto header = gen_packet_header(command::client_register, sizeof(wchar_t)* name_.length());
        _link->write_some(boost::asio::buffer(&header, sizeof( header )));
        _link->write_some(boost::asio::buffer(name_.data(), header.content_length));
    }
}

void client_net_link::operator()() {
    if ( !_link ) {
        return;
    }
    boost::system::error_code error;
    for ( ;; ) {
        auto result = _link->read_some( boost::asio::buffer( _buffer ), error );
        if ( result > 0 ) {
            on_net_packet( _buffer.data(), result );
            continue; // read all packets at once
        }

        if ( error ) {
            if ( error != boost::asio::error::would_block ) {
                disconnect();
                break;
            } else {
                break;
            }
        }
    }
}