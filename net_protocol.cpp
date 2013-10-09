#include "net_protocol.h"
#include "client.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>


const char* net_protocol::on_client_register(const char* from_, const char* to_) {
    if ( _sv ) {
        _sv->on_client_register(this, reinterpret_cast<const wchar_t*>( from_ ), reinterpret_cast<const wchar_t*>( to_ ));
    }
    return to_;
}

const char* net_protocol::on_client_unregister(const char* from_, const char* to_) {
    string_id name_id;
    auto offset = bit_unpack_int(reinterpret_cast<const unsigned char*>(from_), 0, name_id);
    if ( _sv ) {
        _sv->on_client_unregister(this, name_id);
    }
    // round to next full byte
    return from_ + ( ( offset + 7 ) / 8 );
}

const char* net_protocol::on_string_lookup(const char* from_, const char* to_) {
    string_id sid;
    auto offset = bit_unpack_int( reinterpret_cast<const unsigned char*>( from_ ), 0, sid );
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
    auto offset = bit_unpack_int( reinterpret_cast<const unsigned char*>( from_ ), 0, sid );
    offset = bit_unpack_int( reinterpret_cast<const unsigned char*>( from_ ), offset, length );
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
    auto offset = bit_unpack_int( reinterpret_cast<const unsigned char*>( from_ ), 0, name_id );
    from_ += ( offset + 7 ) / 8;
    if ( _cl ) {
        _cl->on_set_name(name_id, reinterpret_cast<const wchar_t*>( from_ ), reinterpret_cast<const wchar_t*>( to_ ));
    }
    return to_;
}

const char* net_protocol::on_remove_name(const char* from_, const char* to_) {
    string_id name_id;
    auto offset = bit_unpack_int( reinterpret_cast<const unsigned char*>( from_ ), 0, name_id );
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

net_protocol::net_protocol()
    : _sv(nullptr)
    , _cl(nullptr)
    , _socket(INVALID_SOCKET) {
        _read_buffer.resize(1024);
}

net_protocol::net_protocol(SOCKET socket_)
    : _sv(nullptr)
    , _cl(nullptr)
    , _socket(socket_) {
        _read_buffer.resize(1024);
}

net_protocol::net_protocol(net_protocol&& other_)
    : net_protocol() {
    *this = std::move(other_);
}

net_protocol& net_protocol::operator=( net_protocol && other_ ) {
    if ( is_open() ) {
        disconnect();
    }

    _sv = std::move(other_._sv);
    _cl = std::move(other_._cl);
    _socket = std::move(other_._socket);
    other_._socket = INVALID_SOCKET;
    _read_buffer = std::move(other_._read_buffer);

    return *this;
}
net_protocol::~net_protocol() {
    if ( is_open() ) {
        disconnect();
    }
}

bool net_protocol::is_open() {
    return _socket != INVALID_SOCKET;
}

bool net_protocol::connect(const sockaddr& peer_) {
    return SOCKET_ERROR != ::connect(_socket, &peer_, sizeof( peer_ ));
}

void net_protocol::disconnect() {
    if ( is_open() ) {
        ::shutdown(_socket, SD_BOTH);
        ::closesocket(_socket);
        _socket = INVALID_SOCKET;
    }
}

bool net_protocol::listen(const sockaddr& port_) {
    _socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if ( _socket == INVALID_SOCKET ) {
        return false;
    }

    BOOL reuse = TRUE;
    if ( ::setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>( &reuse ), sizeof( reuse )) ) {
        BOOST_LOG_TRIVIAL(error) << L"setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>( &reuse ), sizeof( reuse )) failed with: " << WSAGetLastError();
        return false;
    }

    if ( ::bind(_socket, &port_, sizeof( port_ )) ) {
        BOOST_LOG_TRIVIAL(error) << L"bind(_socket, &port_, sizeof( port_ )) failed with: " << WSAGetLastError();
        return false;
    }

    u_long nonblock = TRUE;
    if ( ::ioctlsocket(_socket, FIONBIO, &nonblock) ) {
        BOOST_LOG_TRIVIAL(error) << L"ioctlsocket(_socket, FIONBIO, &nonblock) failed with: " << WSAGetLastError();
        return false;
    }

    if ( ::listen(_socket, SOMAXCONN) == SOCKET_ERROR ) {
        BOOST_LOG_TRIVIAL(error) << L"listen(_socket, SOMAXCONN) failed with: " << WSAGetLastError();
        return false;
    }

    return true;
}

SOCKET net_protocol::accpet() {
    auto result = ::accept(_socket, nullptr, nullptr);
    if ( result == INVALID_SOCKET ) {
        auto error = WSAGetLastError();
        if ( error != WSAEWOULDBLOCK ) {
            BOOST_LOG_TRIVIAL(error) << L"accept(_socket, nullptr, nullptr) failed with: " << error;
        }
    }
    return result;
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

    return any_failed(write(header), write(name_));
}

net_protocol::connection_status net_protocol::unregister_at_server(string_id name_id_) {
    packet_header header = { 0, command::client_unregister };
    header.packet_length = ( bit_pack_int(reinterpret_cast<unsigned char*>( &name_id_ ), 0, name_id_) + 7 ) / 8;

    return any_failed(write(header), write(&name_id_, header.packet_length));
}

net_protocol::connection_status net_protocol::lookup_string(string_id string_id_) {
    packet_header header = { 0, command::string_lookup };
    header.packet_length = ( bit_pack_int( reinterpret_cast<unsigned char*>( &string_id_ ), 0, string_id_ ) + 7 ) / 8;

    return any_failed(write(header), write(&string_id_, header.packet_length));
}

net_protocol::connection_status net_protocol::request_string_resolve(string_id string_id_) {
    return lookup_string(string_id_);
}

net_protocol::connection_status net_protocol::send_string_info(string_id string_id_, const std::wstring& string_) {
    packet_header header = { 0, command::string_info };
    auto id_length = ( bit_pack_int( reinterpret_cast<unsigned char*>( &string_id_ ), 0, string_id_ ) + 7 ) / 8;
    header.packet_length = string_.length() * sizeof(wchar_t) + id_length;

    return any_failed(write(header), write(&string_id_, id_length), write(string_));
}

net_protocol::connection_status net_protocol::send_combat_event(const combat_log_entry& e_) {
    packet_header header = { 0, command::combat_event };
    auto result = compress(e_);
    header.packet_length = ( std::get<1>( result ) + 7 ) / 8;
    auto& buf = std::get<0>( result );

    return any_failed(write(header), write(buf.data(), header.packet_length));
}

net_protocol::connection_status net_protocol::set_name(string_id name_id_, const std::wstring& name_) {
    packet_header header = { 0, command::server_set_name };
    auto id_length = ( bit_pack_int( reinterpret_cast<unsigned char*>( &name_id_ ), 0, name_id_ ) + 7 ) / 8;
    header.packet_length = id_length + name_.length() * sizeof(wchar_t);

    return any_failed(write(header), write(&name_id_, id_length), write(name_));
}

net_protocol::connection_status net_protocol::remove_name(string_id name_id_) {
    packet_header header = { 0, command::server_set_name };
    header.packet_length = ( bit_pack_int( reinterpret_cast<unsigned char*>( &name_id_ ), 0, name_id_ ) + 7 ) / 8;

    return any_failed(write(header), write(&name_id_, header.packet_length));
}

net_protocol::connection_status net_protocol::read_from_port() {
    boost::system::error_code ec;

    std::array<char, 1024> buffer;
    auto bytes = ::recv(_socket, buffer.data(), buffer.size(), 0);

    if ( bytes == SOCKET_ERROR ) {
        auto error = WSAGetLastError();

        if ( error == WSAEWOULDBLOCK ) {
            return connection_status::connected;
        }

        BOOST_LOG_TRIVIAL(error) << L"recv(_socket, buffer.data(), buffer.size(), 0) failed with: " << error;

        return error == WSAETIMEDOUT ? connection_status::timeout : connection_status::closed;
    }

    if ( bytes > 0 ) {
        _read_buffer.insert(end(_read_buffer), begin(buffer), end(buffer));

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