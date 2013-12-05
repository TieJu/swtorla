#pragma once

#include <WinSock2.h>
#include <Windows.h>

#include <vector>
#include <algorithm>

#include "swtor_log_parser.h"

#include "combat_net.h"


class combat_server;
class combat_server_client {
    friend class combat_server;
protected:
    struct state {
        SOCKET                      _socket { INVALID_SOCKET };
        std::vector<unsigned char>  _recive_buffer;
        string_id                   _id;
    };

    state*          _state { nullptr };
    combat_server*  _server { nullptr };

    void on_client_name( const unsigned char* data_, unsigned short length_ );
    void on_string_query( const unsigned char* data_, unsigned short length_ );
    void on_string_result( const unsigned char* data_, unsigned short length_ );
    void on_combet_event( const unsigned char* data_, unsigned short length_ );

    void on_packet( unsigned char cmd_, const unsigned char* data_, unsigned short length_ ) {
        switch ( cmd_ ) {
        case packet_commands::client_name:
            on_client_name( data_, length_ );
            break;
        case packet_commands::string_query:
            on_string_query( data_, length_ );
            break;
        case packet_commands::string_result:
            on_string_result( data_, length_ );
            break;
        case packet_commands::combat_event:
            on_combet_event( data_, length_ );
            break;
        default:
            // report error?
            break;
        }
    }

    bool parse_next_packet( size_t &at_ ) {
        size_t left = _state->_recive_buffer.size() - at_;

        if ( left < sizeof( packet_header ) ) {
            return false;
        }

        auto header = reinterpret_cast<const packet_header*>( _state->_recive_buffer.data() + at_ );
        if ( left < ( header->_length + sizeof( packet_header ) ) ) {
            return false;
        }

        on_packet( header->_command, reinterpret_cast<const unsigned char*>( header + 1 ), header->_length );
        at_ += sizeof(packet_header) + header->_length;
        return true;
    }

    void move_packet_data_to_start( size_t at_ ) {
        std::copy( begin( _state->_recive_buffer ) + at_, end( _state->_recive_buffer ), begin( _state->_recive_buffer ) );
        _state->_recive_buffer.resize( _state->_recive_buffer.size() - at_ );
    }

    static bool is_player( string_id id_ ) {
        return id_ == 0;
    }
    bool is_any_player( string_id id_ );

    // rules:
    // - accept any event when the src is the player
    // - accept no events when the src is any other player (or we count healing etc twice)
    // - accept every thing else
    bool skip_packet( const combat_log_entry& cle_ ) {
        return !is_player( cle_.src )
            && is_any_player( cle_.src );
    }
public:
    combat_server_client( state& state_, combat_server* server_ ) : _state( &state_ ), _server( server_ ) {}
    combat_server_client( const combat_server_client& ) = default;
    combat_server_client() = default;
    ~combat_server_client() = default;
    combat_server_client& operator=( const combat_server_client& ) = default;

    bool try_recive() {
        const size_t recive_length = 1024;
        auto offset = _state->_recive_buffer.size();
        // first make room for new data
        _state->_recive_buffer.resize( recive_length + offset );
        // read the data into the new space
        auto len = ::recv( _state->_socket, reinterpret_cast<char*>( _state->_recive_buffer.data() + offset ), recive_length, 0 );

        if ( len <= 0 ) {
            return false;
        }

        // truncate length to the actual size of all valid data
        _state->_recive_buffer.resize( offset + len );

        // process data blocks
        size_t pos = 0;
        while ( parse_next_packet( pos ) ) {
        }

        // remove processed data
        move_packet_data_to_start( pos );
        return true;
    }

    void send_combat_log_entry( const combat_log_entry_compressed& ce_ ) {
        packet_header hdr { ce_.size( ), packet_commands::combat_event };
        ::send( _state->_socket, reinterpret_cast<const char*>( &hdr ), sizeof( hdr ), 0 );
        ::send( _state->_socket, reinterpret_cast<const char*>( ce_.data() ), ce_.size(), 0 );
    }

    void send_string_result( string_id id_, const std::wstring& str_ ) {
        packet_header hdr { sizeof(id_)+sizeof(wchar_t)* str_.length( ), packet_commands::string_result };
        ::send( _state->_socket, reinterpret_cast<const char*>( &hdr ), sizeof( hdr ), 0 );
        ::send( _state->_socket, reinterpret_cast<const char*>( &id_ ), sizeof( id_ ), 0 );
        ::send( _state->_socket, reinterpret_cast<const char*>( str_.data( ) ), sizeof(wchar_t)* str_.length( ), 0 );
    }
    void send_string_query( string_id id_ ) {
        packet_header hdr { sizeof( id_ ), packet_commands::string_query };
        ::send( _state->_socket, reinterpret_cast<const char*>( &hdr ), sizeof( hdr ), 0 );
        ::send( _state->_socket, reinterpret_cast<const char*>( &id_ ), sizeof( id_ ), 0 );
    }
    void send_player_name( string_id id_, const std::wstring& str_ ) {
        packet_header hdr { sizeof(id_) + sizeof(wchar_t) * str_.length(), packet_commands::client_set };
        ::send( _state->_socket, reinterpret_cast<const char*>( &hdr ), sizeof( hdr ), 0 );
        ::send( _state->_socket, reinterpret_cast<const char*>( &id_ ), sizeof( id_ ), 0 );
        ::send( _state->_socket, reinterpret_cast<const char*>( str_.data() ), sizeof(wchar_t) * str_.length(), 0 );
    }
    void send_player_remove( string_id id_ ) {
        packet_header hdr { sizeof( id_ ), packet_commands::client_remove };
        ::send( _state->_socket, reinterpret_cast<const char*>( &hdr ), sizeof( hdr ), 0 );
        ::send( _state->_socket, reinterpret_cast<const char*>( &id_ ), sizeof( id_ ), 0 );
    }

    bool is_link_to( const state& state_ ) { return _state->_socket == state_._socket; }
    bool operator ==( const combat_server_client& other_ ) const {
        return _state == other_._state;
    }
};

class combat_server {
    friend class combat_server_client;
    std::vector<combat_server_client::state>    _players;
    string_id                                   _next_id {1};
    std::unordered_map<string_id, std::wstring> _string_map;
    std::unordered_map<string_id, std::wstring> _char_map;

    struct encounter {
        std::vector<combat_log_entry>   _entries;
        unsigned                        _begins { 0 };
        unsigned                        _ends { 0 };
    };

    std::vector<encounter>  _encounters;

    void translate( string_id& id_, combat_server_client src_ ) {
        if ( 0 == id_ ) {
            id_ = src_._state->_id;
        }
    }
    void translate( combat_log_entry& cle_, combat_server_client src_ ) {
        translate( cle_.src, src_ );
        translate( cle_.dst, src_ );
    }
    bool is_any_player( string_id id_ ) {
        if ( id_ >= _next_id ) {
            return false;
        }

        auto ref = std::find_if( begin( _players ), end( _players ), [=]( const combat_server_client::state& state_ ) {
            return state_._id == id_;
        } );

        return ref != end( _players );
    }

protected:
    static bool is_combat_begin( const combat_log_entry& cle_ ) {
        return cle_.effect_action == ssc_Event && cle_.effect_type == ssc_EnterCombat;
    }

    static bool is_combat_end( const combat_log_entry& cle_ ) {
        return cle_.effect_action == ssc_Event && cle_.effect_type == ssc_ExitCombat;
    }

    void combat_event( combat_server_client cl_, combat_log_entry cle_ ) {
        if ( _encounters.empty() ) {
            _encounters.push_back( encounter {} );
        }

        translate( cle_, cl_ );

        auto& target = _encounters.back( );
        target._begins += is_combat_begin( cle_ );
        target._ends += is_combat_end( cle_ );

        _encounters.back( )._entries.push_back( cle_ );

        if ( target._begins == target._ends ) {
            _encounters.push_back( encounter {} );
        }

        combat_log_entry_compressed ce { cle_ };
        for ( auto& state : _players ) {
            if ( cl_.is_link_to( state ) ) {
                continue;
            }
            combat_server_client { state, this }.send_combat_log_entry( ce );
        }
    }

    bool is_id_in_use( string_id id_ ) {
        return end(_players ) 
            != std::find_if( begin( _players ), end( _players ), [=]( const combat_server_client::state& state_ ) {
                return state_._id == id_; 
            } );
    }

    string_id get_next_id() {
        string_id id_next = 1;

        for ( ; is_id_in_use( id_next ); ++id_next ) {}

        if ( id_next >= _next_id ) {
            _next_id = id_next + 1;
        }
        return id_next;
    }

    void set_client_name( string_id id_, const std::wstring& name_ ) {
        _char_map[id_] = name_;

        for ( auto& cl : _players ) {
            if ( cl._id == id_ ) {
                continue;
            }
            combat_server_client { cl, this }.send_player_name( id_, name_ );
        }
    }
    void query_string( string_id id_, combat_server_client target_ ) {
        auto at = _string_map.find( id_ );
        if ( end( _string_map ) == at ) {
            for ( auto& cl : _players ) {
                combat_server_client csc { cl, this };
                if ( csc == target_ ) {
                    continue;
                }
                csc.send_string_query( id_ );
            }
        } else {
            target_.send_string_result( at->first, at->second );
        }
    }

    void result_string( string_id id_, const std::wstring& str_, combat_server_client src_ ) {
        auto result = _string_map.insert( std::make_pair( id_, str_ ) );

        if ( result.second ) {
            for ( auto& cl : _players ) {
                combat_server_client { cl, this }.send_string_result( id_, str_ );
            }
        }
    }

public:
    combat_server() = default;
    ~combat_server() = default;

    combat_server_client get_client_from_socket( SOCKET socket_ ) {
        auto ref = std::find_if( begin( _players ), end( _players ), [=]( const combat_server_client::state& state_ ) {
            return state_._socket == socket_;
        } );

        if ( ref == end( _players ) ) {
            ref = _players.insert( _players.end( ), combat_server_client::state {} );
            ref->_socket = socket_;
            ref->_id = get_next_id();
        }

        return combat_server_client { *ref, this };
    }
    void remove_client_by_socket( SOCKET socket_ ) {
        auto ref = std::find_if( begin( _players ), end( _players ), [=]( const combat_server_client::state& state_ ) {
            return state_._socket == socket_;
        } );
        if ( end( _players ) == ref ) {
            return;
        }

        for ( auto& cl : _players ) {
            if ( cl._id == ref->_id ) {
                continue;
            }
            combat_server_client { cl, this }.send_player_remove( ref->_id );
            _char_map.erase( ref->_id );
        }

        _players.erase( ref );
    }

    void close_all_sockets() {
        for ( auto& cl : _players ) {
            // this will later trigger FD_CLOSE events for all sockets
            ::shutdown( cl._socket, SD_BOTH );
        }
    }
};


inline
void combat_server_client::on_client_name( const unsigned char* data_, unsigned short length_ ) {
    if ( length_ % sizeof( wchar_t ) ) {
        return;
    }

    _server->set_client_name( _state->_id, std::wstring { reinterpret_cast<const wchar_t*>( data_ ), reinterpret_cast<const wchar_t*>( data_ + length_ ) });
}

inline
void combat_server_client::on_string_query( const unsigned char* data_, unsigned short length_ ) {
    if ( length_ != sizeof( string_id ) ) {
        return;
    }

    _server->query_string( *reinterpret_cast<const string_id*>( data_ ), *this );
}

inline
void combat_server_client::on_string_result( const unsigned char* data_, unsigned short length_ ) {
    if ( length_ <= sizeof( string_id ) ) {
        return;
    }

    if ( ( length_ - sizeof( string_id ) ) % sizeof( wchar_t ) ) {
        return;
    }

    auto id_ptr = reinterpret_cast<const string_id*>( data_ );
    auto str_ptr = reinterpret_cast<const wchar_t*>( id_ptr + 1 );
    _server->result_string( *id_ptr, std::wstring { str_ptr, reinterpret_cast<const wchar_t*>( data_ + length_ ) }, *this );
}

inline
void combat_server_client::on_combet_event( const unsigned char* data_, unsigned short length_ ) {
    combat_log_entry_compressed clec { data_, length_ };

    combat_log_entry cle = clec;

    if ( skip_packet( cle ) ) {
        return;
    }

    _server->combat_event( *this, cle );
}

inline
bool combat_server_client::is_any_player( string_id id_ ) {
    return _server->is_any_player( id_ );
}