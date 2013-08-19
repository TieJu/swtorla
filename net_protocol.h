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

class net_protocol_client_interface {
protected:
    net_protocol_client_interface() {}

public:
    virtual ~net_protocol_client_interface() {}

    virtual bool on_string_lookup(string_id ident_) = 0;
    virtual bool on_string_info(string_id ident_, const wchar_t* string_start_, const wchar_t* string_end_) = 0;
    virtual bool on_set_name(string_id ident_, const wchar_t* name_start_, const wchar_t* name_end_) = 0;
    virtual bool on_remove_name(string_id ident_) = 0;
    virtual bool on_combat_event(const combat_log_entry& e_) = 0;
};

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
    net_protocol_client_interface*  _cl;
    boost::asio::ip::tcp::socket    _socket;
    boost::asio::ip::tcp::acceptor  _accept;
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
    template<typename Buffers>
    connection_status write_buffes(Buffers buffers_);

public:
    net_protocol(boost::asio::io_service& ios_);
    net_protocol(boost::asio::io_service& ios_, boost::asio::ip::tcp::socket && connection_);
    template<typename Iterator>
    Iterator connect(Iterator from_, Iterator to_) {
        boost::system::error_code ec;
        for ( ; from_ != to_; ++from_ ) {
            if ( _socket.connect(*from_, ec) ) {
                break;
            }
        }
        return from_;
    }

    void disconnect();
    bool listen(const boost::asio::ip::tcp::endpoint& end_point_);
    bool accpet(boost::asio::ip::tcp::socket& target_);
    void server(net_protocol_server_interface* is_);
    void client(net_protocol_client_interface* ic_);
    connection_status register_at_server(const std::wstring& name_);
    connection_status unregister_at_server(string_id name_id_);
    connection_status lookup_string(string_id string_id_);
    connection_status request_string_resolve(string_id string_id_);
    connection_status send_string_info(string_id string_id_, const std::wstring& string_);
    connection_status send_combat_event(const combat_log_entry& e_);
    connection_status set_name(string_id name_id_, const std::wstring& name_);
    connection_status remove_name(string_id name_id_);
    connection_status read_from_port();
    boost::asio::ip::tcp::acceptor& server_port() {
        return _accept;
    }
    boost::asio::ip::tcp::socket& client_port() {
        return _socket;
    }
};