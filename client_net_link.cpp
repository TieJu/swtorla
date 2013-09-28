#include "app.h"

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
    case command::combat_event:
        _ci->on_combat_event(this, std::get<0>(uncompress(data_begin_, 0)));
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

bool client_net_link::init_link(const std::string& peer_, const std::string& port_) {
    BOOST_LOG_TRIVIAL(debug) << L"looking up remote server address for " << peer_ << L" " << port_;
    boost::asio::ip::tcp::resolver tcp_lookup(_ci->get_io_service());
    boost::asio::ip::tcp::resolver::query remote_query(peer_, port_);
    auto list = tcp_lookup.resolve(remote_query);

    BOOST_LOG_TRIVIAL(debug) << L"connecting to remote server";
    boost::asio::ip::tcp::socket socket(_ci->get_io_service());
    boost::system::error_code error;
    boost::asio::ip::tcp::resolver::iterator lend;
    for ( ; lend != list; ++list ) {
        if ( !socket.connect(*list, error) ) {
            BOOST_LOG_TRIVIAL(debug) << L"connected to " << list->endpoint();
            break;
        }
    }

    if ( error ) {
        BOOST_LOG_TRIVIAL(error) << L"unable to connect to remote server";
        return false;
    }

    _link->non_blocking(true);
    _ci->on_connected_to_server(this);
    return true;
}

void client_net_link::shutdown_link() {
    if ( !_link ) {
        return;
    }

    _link->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
    _link->close();
    _ci->on_disconnected_from_server(this);
}

void client_net_link::run() {
    boost::system::error_code error;
    boost::array<char, 1024> read_buffer;
    state r_state = state::sleep;

    for ( ;; ) {
        r_state = wait(r_state);

        if ( state::sleep == r_state || state::shutdown == r_state ) {
            shutdown_link();
        }

        if ( state::shutdown == r_state ) {
            break;
        }

        if ( state::init == r_state ) {
            if ( !init_link(_peer, _peer_port) ) {
                change_state(state::sleep);
                r_state = state::sleep;
            } else {
                r_state = state::run;
            }
        }

        while ( state::run == r_state ) {
            auto result = _link->read_some(boost::asio::buffer(read_buffer), error);
            if ( result > 0 ) {
                on_net_packet(read_buffer.data(), result);
                continue; // dont wait, just try to get next data set
            }

            if ( error ) {
                if ( error != boost::asio::error::would_block ) {
                    // error...
                    break;
                }
            }
            // only block if no data is available
            r_state = wait_for(r_state, std::chrono::milliseconds(25));
        }
    }
}

client_net_link::client_net_link()
    : _ci(nullptr) {
}

client_net_link::client_net_link(app& ci_)
    : _ci(&ci_)
    , _link(new boost::asio::ip::tcp::socket(ci_.get_io_service())) {
}

client_net_link::client_net_link(client_net_link && other_)
    : client_net_link() {
    *this = std::move(other_);
}

client_net_link::~client_net_link() {
}

client_net_link& client_net_link::operator=( client_net_link && other_ ) {
    active<client_net_link>::operator=( std::move(other_) );
    net_link_base<client_net_link>::operator=( std::move(other_) );
    _ci = other_._ci;
    other_._ci = nullptr;
    _link = std::move(other_._link);
    _peer = std::move(other_._peer);
    _peer_port = std::move(other_._peer_port);
    return *this;
}

void client_net_link::connect(const std::string& name_, const std::string& port_) {
    if ( !_link ) {
        throw std::runtime_error("tryed to connect with an uninitialized client_net_link instance");
    }
    if ( !is_runging() ) {
        start();
    }
    _peer = name_;
    _peer_port = port_;
    change_state(state::init);
}
void client_net_link::disconnect() {
    change_state(state::sleep);
}

void client_net_link::register_at_server(const std::wstring& name_) {
    auto header = gen_packet_header(command::client_register, sizeof(wchar_t) * name_.length());
    _link->write_some(boost::asio::buffer(&header, sizeof( header )));
    _link->write_some(boost::asio::buffer(name_.data(), header.content_length));
}

void client_net_link::unregister_at_server(string_id name_id_) {
    auto header = gen_packet_header(command::client_unregister, sizeof( name_id_ ));
    _link->write_some(boost::asio::buffer(&header, sizeof( header )));
    _link->write_some(boost::asio::buffer(&name_id_, sizeof( name_id_ )));
}

void client_net_link::get_string_value(string_id string_id_) {
    auto header = gen_packet_header(command::string_lookup, sizeof( string_id_ ));
    _link->write_some(boost::asio::buffer(&header, sizeof( header )));
    _link->write_some(boost::asio::buffer(&string_id_, sizeof( string_id_ )));
}

void client_net_link::send_string_value(string_id string_id_, const std::wstring& value_) {
    auto header = gen_packet_header(command::string_info, sizeof( string_id_ ) + sizeof(wchar_t) * value_.length());
    _link->write_some(boost::asio::buffer(&header, sizeof( header )));
    _link->write_some(boost::asio::buffer(&string_id_, sizeof( string_id_ )));
    _link->write_some(boost::asio::buffer(value_.data(), sizeof(wchar_t) * value_.length()));
}

void client_net_link::send_combat_event(const combat_log_entry& event_) {
    auto compressed_event = compress(event_);
    auto length = ( 7 + std::get<1>( compressed_event ) ) / 8;
    auto& data = std::get<0>( compressed_event );
    auto header = gen_packet_header(command::combat_event, length);
    _link->write_some(boost::asio::buffer(&header, sizeof( header )));
    _link->write_some(boost::asio::buffer(data.data(), header.content_length));
}