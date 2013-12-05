#pragma once

#include <WinSock2.h>
#include <Windows.h>

#include <vector>
#include <algorithm>

#include "socket_api.h"
#include "socket.h"
#include "socket_address.h"
#include "swtor_log_parser.h"
#include "combat_net.h"
#include "player_db.h"

class combat_client {
    c_socket                    _server;
    std::vector<unsigned char>  _recive_buffer;
    bool                        _name_send { false };
    player_db*                  _player_db { nullptr };

    bool parse_next_packet( size_t at ) { return false; }
    void move_packet_data_to_start( size_t at ) { }
    
    static bool is_player( string_id id_ ) {
        return id_ == 0;
    }
    bool is_any_player( string_id id_ ) { 
        return _player_db->is_player_name_set( id_ );
    }

    bool skip_combat_event_send( const combat_log_entry& event_ ) {
        // todo: extend this rules to only send combat relevant events
        return !is_player( event_.src )
            && is_any_player( event_.src );
    }
    void send_combat_event( const combat_log_entry& event_ ) {
        if ( !_name_send ) {
            send_player_name();
        }

        combat_log_entry_compressed compressed { event_ };

        packet_header hdr { compressed.size(), packet_commands::combat_event };
        _server.send( hdr );
        _server.send( compressed.data(), compressed.size() );
    }
    void send_player_name() {
        auto name = _player_db->get_player_name();
        packet_header hdr { name.length() * sizeof( wchar_t ), packet_commands::client_name };
        _server.send( hdr );
        _server.send( name.data(), name.length() * sizeof( wchar_t ) );
        _name_send = true;
    }

public:
    combat_client() = default;
    combat_client( const combat_client& ) = delete;
    combat_client( combat_client&& other_ ) { *this = std::move( other_ ); }
    combat_client( player_db& pdb_ ) : _player_db( &pdb_ ) {}
    ~combat_client() = default;
    combat_client& operator=( const combat_client& ) = delete;
    combat_client& operator=( combat_client&& other_ ) {
        _server = std::move( other_._server );
        _recive_buffer = std::move( other_._recive_buffer );
        _name_send = std::move( other_._name_send );
        _player_db = std::move( other_._player_db );
        return *this;
    }
    void set_connection( c_socket&& socket_ ) {
        _server = std::move( socket_ );
    }
    void disconnect_from_server() { _server.shutdown( SD_BOTH ); }
    void close_socket() { _server.reset(); }

    void on_combat_event( const combat_log_entry& event_ ) {

        if ( !skip_combat_event_send( event_ ) ) {
            send_combat_event( event_ );
        }
    }
    bool try_recive( ) {
        const size_t recive_length = 1024;
        auto offset = _recive_buffer.size( );
        // first make room for new data
        _recive_buffer.resize( recive_length + offset );
        // read the data into the new space
        auto len = _server.recive(  _recive_buffer.data( ) + offset, recive_length, 0 );

        if ( len <= 0 ) {
            return false;
        }

        // truncate length to the actual size of all valid data
        _recive_buffer.resize( offset + len );

        // process data blocks
        size_t pos = 0;
        while ( parse_next_packet( pos ) ) {
        }

        // remove processed data
        move_packet_data_to_start( pos );
        return true;
    }

    void send_string_query( string_id id_ ) {}
};