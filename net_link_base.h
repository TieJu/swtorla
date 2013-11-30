#pragma once

#include "socket.h"

#include <vector>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

enum class command : char {
    // client connects to server and submits players name (obtained from log)
    client_register,

    string_lookup,
    // client response to 'string_lookup'
    string_info,

    // combat event, can be send by client or server
    combat_event,

    //UPDATE? if name is empty, client has been removed
    server_set_name,
};

template<typename Type>
inline void send( c_socket& s_, const Type& value_ ) {
    s_.send( value_ );
}

inline void send( c_socket& s_, const void* buf_, size_t length_ ) {
    s_.send( buf_, length_ );
}

// base protocol stuff, mostly packet parsing
template<typename Derived>
class net_link_base {
    std::vector<char>   _data_buffer;

#pragma pack(push)
#pragma pack(1)
    struct packet_header {
        unsigned short      content_length;
        command             cmd;
    };
#pragma pack(pop)

    void process_data() {
        const auto end_read = _data_buffer.data() + _data_buffer.size();
        auto read_pos = _data_buffer.data();
        auto last_finished_part = read_pos;
#define check_left(req_size_) ( ( end_read - read_pos ) >= ( req_size_ ) )

        for (;;) {
            // enough space left to hold a packet header?
            if ( !check_left(sizeof(packet_header)) ) {
                break;
            }

            auto header = reinterpret_cast<const packet_header*>( read_pos );
            read_pos += sizeof( packet_header );

            if ( !check_left(header->content_length) ) {
                break;
            }
            static_cast<Derived*>( this )->on_net_link_command(header->cmd, read_pos, read_pos + header->content_length);
            read_pos += header->content_length;
            last_finished_part = read_pos;
        }
        // move the rest of the data set to the begining of the buffer
        std::copy(last_finished_part, end_read, _data_buffer.data());
        // drop unused part
        _data_buffer.resize(end_read - last_finished_part);
#undef check_left
    }

protected:
    void on_net_packet( const char* data_, size_t length_ ) {
        BOOST_LOG_TRIVIAL( debug ) << L"void net_link_base::on_net_packet(" << (void*)data_ << L", " << length_ << L");";
        _data_buffer.insert(_data_buffer.end(), data_, data_ + length_);

        process_data();
    }

    packet_header gen_packet_header(command command_, size_t length_) {
        if ( length_ > 0xFFFF ) {
            throw std::runtime_error("net_link_base::gen_packet_header packet data too large!");
        }
        return packet_header { (unsigned short)(length_), command_ };
    }

    net_link_base& operator=( net_link_base && other_ ) {
        _data_buffer = std::move(other_._data_buffer);
        return *this;
    }

public:
    void get_string_value(string_id string_id_) {
        auto self = static_cast<Derived*>( this );
        if ( self->is_link_active( ) ) {
            auto& link = self->get_link( );
            auto header = gen_packet_header(command::string_lookup, sizeof( string_id_ ));
            send( link, header );
            send( link, string_id_ );
        }
    }
    void send_string_value(string_id string_id_, const std::wstring& value_) {
        auto self = static_cast<Derived*>( this );
        if ( self->is_link_active( ) ) {
            auto& link = self->get_link( );
            auto header = gen_packet_header(command::string_info, sizeof( string_id_ ) + sizeof(wchar_t) * value_.length());
            send( link, header );
            send( link, string_id_ );
            send( link, value_.data(), sizeof(wchar_t) * value_.length() );
        }
    }
    void send_combat_event(const combat_log_entry_compressed& event_) {
        auto self = static_cast<Derived*>( this );
        if ( self->is_link_active( ) ) {
            auto& link = self->get_link( );
            auto header = gen_packet_header( command::combat_event, event_.size() );
            send( link, header );
            send( link, event_.data(), event_.size() );
        }
    }
    template<typename U>
    void send_combat_event( const combat_log_entry_compressed_wrap<U>& event_ ) {
        auto self = static_cast<Derived*>( this );
        if ( self->is_link_active( ) ) {
            auto& link = self->get_link( );
            auto header = gen_packet_header( command::combat_event, event_.size() );
            send( link, header );
            send( link, event_.data(), event_.size() );
        }
    }
};