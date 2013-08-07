#pragma once

#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <boost/asio.hpp>
#include <boost/array.hpp>

#include "swtor_log_parser.h"

#include "active.h"

// max length = size * 8 / 7
// 64 bit = 8 * 8 / 7

template<typename IntType>
inline IntType unpack_int(char*& from_, char* to_) {
    IntType result = 0;
    int shift = 0;
    while ( shift < sizeof(IntType ) * 8 && from_ < to_ ) {
        result |= ( ( *from_ ) & 0x7F ) << shift;
        shift += 7;
        if ( ( *from_ ) & 0x80 ) {
            ++from_;
        } else {
            ++from_;
            break;
        }
    }
    return result;
}

template<>
inline bool unpack_int<bool>(char*& from_, char* to_) {
    return *from_++ ? true : false;
}

template<typename IntType>
inline char* pack_int(char* from_, char* to_, IntType value_) {
    do {
        *from_ = char( value_ & 0x7F );
        value_ >>= 7;
        if ( value_ ) {
            *from_ |= 0x80;
        }
        ++from_;
    } while ( value_ && from_ < to_ );
    return from_;
}

inline char* pack_int(char* from_, char* to_, bool value_) {
    *from_ = value_ ? 1 : 0;
    return from_ + 1;
}


class net_link : protected active<net_link> {
public:
    typedef std::function<void( string_id, const wchar_t*, const wchar_t* )>    local_string_handler_t;
    typedef std::function<void(const combat_log_entry&)>                        log_entry_handler_t;

private:
    enum class packet_type : char {
        string,
        log,
    };

    local_string_handler_t                      _on_string;
    log_entry_handler_t                         _on_entry;
    boost::asio::ip::tcp::socket                _port;
    boost::asio::ip::tcp::resolver::iterator    _target_from;
    boost::asio::ip::tcp::resolver::iterator    _target_to;

    struct packet_base {
        packet_type     type;
    };

    struct packet_string : packet_base {
        string_id       id;
        unsigned        length;
        wchar_t         str[1];
    };

    struct packet_log : packet_base {
        combat_log_entry    entry;
    };

    bool unpack_log(char*& from_, char* to_) {
        if ( to_ - from_ < sizeof( packet_log ) ) {
            return false;
        }

        _on_entry(reinterpret_cast<const packet_log*>( from_ )->entry);

        from_ += sizeof( packet_log );
        return true;
    }

    bool unpack_string(char*& from_, char* to_) {
        if ( to_ - from_ < sizeof( packet_string ) ) {
            return false;
        }

        auto pack = reinterpret_cast<const packet_string*>( from_ );
        auto total_size = sizeof( packet_string ) + ( pack->length - 1 ) * sizeof(wchar_t);

        if ( to_ - from_ < total_size ) {
            return false;
        }

        _on_string(pack->id, pack->str, pack->str + pack->length);
        from_ += total_size;
        return true;
    }

    char* unpack(char* from_, char* to_) {
        auto start = from_;

        while ( from_ < to_ ) {
            auto head = reinterpret_cast<packet_base*>( from_ );

            if ( head->type == packet_type::log ) {
                if ( !unpack_log(from_, to_) ) {
                    // move left over stuff to the start
                    std::copy(from_, to_, start);
                    return from_;
                }
            } else if ( head->type == packet_type::string ) {
                if ( !unpack_string(from_, to_) ) {
                    // move left over stuff to the start
                    std::copy(from_, to_, start);
                    return from_;
                }
            }
        }

        return start;
    }

    void run() {
        boost::system::error_code error;
        boost::array<char, 4 * 1024> buffer;
        auto t_state = state::sleep;
        auto buffer_from = buffer.data();
        auto buffer_to = buffer_from + buffer.size();
        auto buffer_write_start = buffer_from;
        size_t bytes;

        for ( ;; ) {
            if ( t_state == state::sleep ) {
                if ( _port.is_open() ) {
                    _port.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
                    _port.close();
                }
                t_state = wait(t_state);
            } else {
                t_state = wait_for(t_state, std::chrono::milliseconds(10));
            }

            if ( t_state == state::shutown ) {
                break;
            }

            if ( t_state == state::init ) {
                for ( ; _target_from != _target_to; ++_target_from ) {
                    if ( !_port.connect(*_target_from, error) ) {
                        change_state(state::run);
                        t_state = state::run;
                        break;
                    }
                }
            }

            if ( t_state == state::run ) {
                bytes = _port.read_some(boost::asio::buffer(buffer_write_start, buffer_to - buffer_write_start), error);
                buffer_write_start = unpack(buffer_from, buffer_from + bytes);
            }
        }
    }

    boost::asio::const_buffers_1 pack(string_id id_, const std::wstring& local_value_) {}
    boost::asio::const_buffers_1 pack(const combat_log_entry& entry_) {}


public:
    net_link(boost::asio::io_service& io_service_) : _port(io_service_) {}
    // transmits string info to peer
    void string(string_id id_, const std::wstring& local_value_) {
        boost::system::error_code error;

        _port.write_some(pack(id_,local_value_), error);
    }
    // transmits log entry info to peer
    void log(const combat_log_entry& entry_) {
        boost::system::error_code error;

        _port.write_some(pack(entry_), error);
    }

    void close() {
        change_state(state::sleep);
    }

    void connect(boost::asio::ip::tcp::resolver::iterator from_, boost::asio::ip::tcp::resolver::iterator to_) {
        _target_from = from_;
        _target_to = to_;
        change_state(state::init);
    }
};