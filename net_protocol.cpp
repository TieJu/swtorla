#include "net_protocol.h"
#include "client.h"

const char* net_protocol::on_client_register(const char* from_, const char* to_) {
    if ( _sv ) {
        _sv->on_client_register(this, reinterpret_cast<const wchar_t*>( from_ ), reinterpret_cast<const wchar_t*>( to_ ));
    }
    return to_;
}

const char* net_protocol::on_client_unregister(const char* from_, const char* to_) {
    string_id name_id;
    auto offset = bit_unpack_int(from_, 0, name_id);
    if ( _sv ) {
        _sv->on_client_unregister(this, name_id);
    }
    // round to next full byte
    return from_ + ( ( offset + 7 ) / 8 );
}

const char* net_protocol::on_string_lookup(const char* from_, const char* to_) {
    string_id sid;
    auto offset = bit_unpack_int(from_, 0, sid);
    if ( _sv ) {
        _sv->on_string_lookup(this, sid);
    }
    if ( _cl ) {
        _cl->on_string_lookup(sid);
    }
    // round to next full byte
    return from_ + ( ( offset + 7 ) / 8 );
}

const char* net_protocol::on_string_info(const char* from_, const char* to_) {
    string_id sid;
    size_t length;
    auto offset = bit_unpack_int(from_, 0, sid);
    offset = bit_unpack_int(from_, offset, length);
    from_ += ( offset + 7 ) / 8;
    if ( _sv ) {
        _sv->on_string_info(this, sid, reinterpret_cast<const wchar_t*>( from_ ), reinterpret_cast<const wchar_t*>( from_ ) + length);
    }
    if ( _cl ) {
        _cl->on_string_info(sid, reinterpret_cast<const wchar_t*>( from_ ), reinterpret_cast<const wchar_t*>( from_ ) + length);
    }
    return from_ + length * sizeof(wchar_t);
}

const char* net_protocol::on_combat_event(const char* from_, const char* to_) {
    auto result = uncompress(from_, 0);
    if ( _sv ) {
        _sv->on_combat_event(this, std::get<0>( result ));
    }
    if ( _cl ) {
        _cl->on_combat_event(std::get<0>( result ));
    }
    // round to next full byte
    return from_ + ( ( std::get<1>( result ) + 7 ) / 8 );
}

const char* net_protocol::on_set_name(const char* from_, const char* to_) {
    string_id name_id;
    auto offset = bit_unpack_int(from_, 0, name_id);
    from_ += ( offset + 7 ) / 8;
    if ( _cl ) {
        _cl->on_set_name(name_id, reinterpret_cast<const wchar_t*>( from_ ), reinterpret_cast<const wchar_t*>( to_ ));
    }
    return to_;
}

const char* net_protocol::on_remove_name(const char* from_, const char* to_) {
    string_id name_id;
    auto offset = bit_unpack_int(from_, 0, name_id);
    from_ += ( offset + 7 ) / 8;
    if ( _cl ) {
        _cl->on_remove_name(name_id);
    }
    return from_;
}

const char* net_protocol::on_raw_packet(const char* from_, const char* to_) {
    if ( to_ - from_ < sizeof( packet_header ) ) {
        return from_;
    }

    auto ph = reinterpret_cast<const packet_header*>( from_ );
    if ( to_ - from_ < ph->packet_length + sizeof( packet_header ) ) {
        return from_;
    }

    auto data_begin = reinterpret_cast<const  char*>( ph + 1 );
    auto data_end = data_begin + ph->packet_length;
    return on_packet(ph->command, data_begin, data_end);
}

const char* net_protocol::on_packet(command cmd_,const char* from_, const char* to_) {
    switch ( cmd_ ) {
    case command::client_register:      return on_client_register(from_, to_);
    case command::client_unregister:    return on_client_unregister(from_, to_);
    case command::string_lookup:        return on_string_lookup(from_, to_);
    case command::string_info:          return on_string_info(from_, to_);
    case command::server_set_name:      return on_set_name(from_, to_);
    case command::server_remove_name:   return on_remove_name(from_, to_);
    case command::combat_event:         return on_combat_event(from_, to_);
    }
}

template<typename Buffers>
net_protocol::connection_status net_protocol::write_buffes(Buffers buffers_) {
    boost::system::error_code ec;
    // using write here, wee need to wait until all bytes are copied to the socket
    boost::asio::write(_socket, buffers_, ec);

    if ( !ec ) {
        if ( ec == boost::asio::error::eof ) {
            return connection_status::closed;
        }// else if ( ec == boost::asio::error::timed_out ) {
        return connection_status::timeout;
        //}
    }
    return connection_status::connected;

}

net_protocol::net_protocol(boost::asio::io_service& ios_)
    : _sv(nullptr)
    , _cl(nullptr)
    , _socket(ios_)
    , _accept(ios_) {
        _read_buffer.resize(1024);
}

net_protocol::net_protocol(boost::asio::io_service& ios_, boost::asio::ip::tcp::socket && connection_)
    : _sv(nullptr)
    , _cl(nullptr)
    , _socket(std::move(connection_))
    , _accept(ios_) {
        _read_buffer.resize(1024);
}

void net_protocol::disconnect() {
    _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
    _socket.close();
}

bool net_protocol::listen(const boost::asio::ip::tcp::endpoint& end_point_) {
    boost::system::error_code ec;
    if ( _accept.open(end_point_.protocol(), ec) ) {
        _accept.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        if ( _accept.bind(end_point_, ec) ) {
            _accept.io_control(boost::asio::ip::tcp::acceptor::non_blocking_io(true));
            if ( _accept.listen(boost::asio::ip::tcp::socket::max_connections, ec) ) {
                return true;
            }
        }
    }
    return false;
}

bool net_protocol::accpet(boost::asio::ip::tcp::socket& target_) {
    boost::system::error_code ec;
    return bool( _accept.accept(target_, ec) );
}

void net_protocol::server(net_protocol_server_interface* is_) {
    _sv = is_;
}

void net_protocol::set_client(client* ic_) {
    _cl = ic_;
}

net_protocol::connection_status net_protocol::register_at_server(const std::wstring& name_) {
    packet_header header = { 0, command::client_register };
    header.packet_length = name_.length() * sizeof(wchar_t);

    boost::array<boost::asio::const_buffer, 3> bufs =
    { boost::asio::const_buffer(&header, sizeof( header ))
    , boost::asio::const_buffer(name_.c_str(), name_.length() * sizeof(wchar_t))
    };

    return write_buffes(bufs);
}

net_protocol::connection_status net_protocol::unregister_at_server(string_id name_id_) {
    packet_header header = { 0, command::client_unregister };
    header.packet_length = ( bit_pack_int(reinterpret_cast<char*>( &name_id_ ), 0, name_id_) + 7 ) / 8;

    boost::array<boost::asio::const_buffer, 3> bufs =
    { boost::asio::const_buffer(&header, sizeof( header ))
    , boost::asio::const_buffer(&name_id_, header.packet_length)
    };

    return write_buffes(bufs);
}

net_protocol::connection_status net_protocol::lookup_string(string_id string_id_) {
    packet_header header = { 0, command::string_lookup };
    header.packet_length = ( bit_pack_int(reinterpret_cast<char*>( &string_id_ ), 0, string_id_) + 7 ) / 8;

    boost::array<boost::asio::const_buffer, 3> bufs =
    { boost::asio::const_buffer(&header, sizeof( header ))
    , boost::asio::const_buffer(&string_id_, header.packet_length)
    };

    return write_buffes(bufs);
}

net_protocol::connection_status net_protocol::request_string_resolve(string_id string_id_) {
    return lookup_string(string_id_);
}

net_protocol::connection_status net_protocol::send_string_info(string_id string_id_, const std::wstring& string_) {
    packet_header header = { 0, command::string_info };
    auto id_length = ( bit_pack_int(reinterpret_cast<char*>( &string_id_ ), 0, string_id_) + 7 ) / 8;
    header.packet_length = ( string_.length() + 1 ) * sizeof(wchar_t)+ id_length;

    boost::array<boost::asio::const_buffer, 3> bufs =
    { boost::asio::const_buffer(&header, sizeof( header ))
    , boost::asio::const_buffer(&string_id_, id_length)
    , boost::asio::const_buffer(string_.c_str(), ( string_.length() + 1 ) *sizeof(wchar_t))
    };

    return write_buffes(bufs);
}

net_protocol::connection_status net_protocol::send_combat_event(const combat_log_entry& e_) {
    packet_header header = { 0, command::combat_event };
    auto result = compress(e_);
    header.packet_length = ( std::get<1>( result ) + 7 ) / 8;
    auto& buf = std::get<0>( result );

    boost::array<boost::asio::const_buffer, 3> bufs =
    { boost::asio::const_buffer(&header, sizeof( header ))
    , boost::asio::const_buffer(buf.data(), header.packet_length)
    };

    return write_buffes(bufs);
}

net_protocol::connection_status net_protocol::set_name(string_id name_id_, const std::wstring& name_) {
    packet_header header = { 0, command::server_set_name };
    auto id_length = ( bit_pack_int(reinterpret_cast<char*>( &name_id_ ), 0, name_id_) + 7 ) / 8;
    header.packet_length = id_length + ( name_.length() + 1 ) * sizeof(wchar_t);

    boost::array<boost::asio::const_buffer, 3> bufs =
    { boost::asio::const_buffer(&header, sizeof( header ))
    , boost::asio::const_buffer(&name_id_, id_length)
    , boost::asio::const_buffer(name_.c_str(), name_.length() *sizeof(wchar_t))
    };

    return write_buffes(bufs);
}

net_protocol::connection_status net_protocol::remove_name(string_id name_id_) {
    packet_header header = { 0, command::server_set_name };
    header.packet_length = ( bit_pack_int(reinterpret_cast<char*>( &name_id_ ), 0, name_id_) + 7 ) / 8;

    boost::array<boost::asio::const_buffer, 3> bufs =
    { boost::asio::const_buffer(&header, sizeof( header ))
    , boost::asio::const_buffer(&name_id_, header.packet_length)
    };

    return write_buffes(bufs);

}

net_protocol::connection_status net_protocol::read_from_port() {
    boost::system::error_code ec;

    boost::array<char, 1024> rb;
    auto bytes = _socket.read_some(boost::asio::buffer(rb), ec);

    if ( ec ) {
        if ( ec == boost::asio::error::eof ) {
            return connection_status::closed;
        }// else if ( ec == boost::asio::error::timed_out ) {
        return connection_status::timeout;
        //}
    }

    if ( bytes > 0 ) {
        _read_buffer.insert(end(_read_buffer), rb.begin(), rb.end());

        const auto* read_start = _read_buffer.data();
        const auto* read_end = read_start + _read_buffer.size();
        auto pos = on_raw_packet(read_start, read_end);
        while ( pos != read_start ) {
            read_start = pos;
            pos = on_raw_packet(read_start, read_end);
        }

        if ( read_start != _read_buffer.data() ) {
            std::copy(read_start, read_end, _read_buffer.data());
            _read_buffer.resize(read_end - read_start);
        }
    }

    return connection_status::connected;
}