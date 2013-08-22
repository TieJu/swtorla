#pragma once

#include "swtor_log_parser.h"
#include "int_compress.h"

#include <boost/optional.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>

class net_protocol;

class net_protocol_server_interface {
protected:
    net_protocol_server_interface() {}

public:
    virtual ~net_protocol_server_interface() {}

    virtual bool on_client_register(net_protocol* client_,const wchar_t* name_start_, const wchar_t* name_end_) = 0;
    virtual bool on_client_unregister(net_protocol* client_, string_id ident_) = 0;
    virtual bool on_string_lookup(net_protocol* client_, string_id ident_) = 0;
    virtual bool on_string_info(net_protocol* client_, string_id ident_, const wchar_t* string_start_, const wchar_t* string_end_) = 0;
    virtual bool on_combat_event(net_protocol* client_, const combat_log_entry& e_) = 0;
};

class client;

class net_protocol {
public:
    enum class connection_status {
        connected,
        closed,
        timeout,
    };

private:
    /*
    ** packet format:
    ** [2 bytes] [unsigned short] packet length
    ** [1 byte] [unsigned char] command id
    ** [packet length * bytes] [byte] contents
    */
    enum class command : char {
        // client connects to server and submits players name (obtained from log)
        client_register,
        // client disconnects or has loged onto different char and removed it self
        client_unregister,

        string_lookup,
        // client response to 'string_lookup'
        string_info,

        // combat event, can be send by client or server
        combat_event,

        server_set_name,
        server_remove_name,
    };

#pragma pack(push)
#pragma pack(1)
    struct packet_header {
        unsigned short  packet_length;
        command         command;
    };
#pragma pack(pop)
    net_protocol_server_interface*  _sv;
    client*                         _cl;
    SOCKET                          _socket;
    std::vector<char>               _read_buffer;

    const char* on_client_register(const char* from_, const char* to_);
    const char* on_client_unregister(const char* from_, const char* to_);
    const char* on_string_lookup(const char* from_, const char* to_);
    const char* on_string_info(const char* from_, const char* to_);
    const char* on_combat_event(const char* from_, const char* to_);
    const char* on_set_name(const char* from_, const char* to_);
    const char* on_remove_name(const char* from_, const char* to_);
    const char* on_raw_packet(const char* from_, const char* to_);
    const char* on_packet(command cmd_,const char* from_, const char* to_);
    template<typename Type>
    connection_status write(const Type& value_) {
        return write(&value_, sizeof( value_ ));
    }
    connection_status write(const packet_header& header_) {
        return write(&header_, sizeof( header_ ));
    }
    connection_status write(const std::wstring& str_) {
        return write(str_.c_str(), str_.length() * sizeof(wchar_t));
    }
    connection_status write(const void* buffer_, size_t length_) {
        auto len = ::send(_socket, reinterpret_cast<const char*>( buffer_ ), length_, 0);

        if ( len == SOCKET_ERROR ) {
            switch ( WSAGetLastError() ) {
            case WSAETIMEDOUT:
                return connection_status::timeout;
            default:
                return connection_status::closed;
            }
        }
        return connection_status::connected;
    }

    static connection_status any_failed(connection_status first_) {
        return first_;
    }
    static connection_status any_failed(connection_status first_, connection_status second_) {
        if ( first_ != connection_status::connected ) {
            return first_;
        }
        return second_;
    }
    static connection_status any_failed(connection_status first_, connection_status second_, connection_status third_) {
        if ( first_ != connection_status::connected ) {
            return first_;
        }
        return any_failed(second_, third_);
    }

public:
    net_protocol();
    net_protocol(SOCKET socket_);
    net_protocol(net_protocol && other_);
    net_protocol& operator=(net_protocol&& other_);
    ~net_protocol();

    bool is_open();
    bool connect(const sockaddr& peer_);
    void disconnect();
    bool listen(const sockaddr& port_);
    SOCKET accpet();
    void server(net_protocol_server_interface* is_);
    void set_client(client* ic_);
    connection_status register_at_server(const std::wstring& name_);
    connection_status unregister_at_server(string_id name_id_);
    connection_status lookup_string(string_id string_id_);
    connection_status request_string_resolve(string_id string_id_);
    connection_status send_string_info(string_id string_id_, const std::wstring& string_);
    connection_status send_combat_event(const combat_log_entry& e_);
    connection_status set_name(string_id name_id_, const std::wstring& name_);
    connection_status remove_name(string_id name_id_);
    connection_status read_from_port();
};