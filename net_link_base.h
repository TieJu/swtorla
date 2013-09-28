#pragma once

#include <vector>

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
};

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
    void on_net_packet(const char* data_, size_t length_) {
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
};