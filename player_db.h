#pragma once

#include <string>
#include <vector>

#include "swtor_log_parser.h"

#ifdef max
#pragma push_macro("max")
#undef max
#define POP_MAX
#endif

class app;

class player_db {
    std::vector<std::wstring>   _players;
    app*                        _app { nullptr };

    void ensure_index_is_present( size_t index_ ) {
        _players.resize( std::max( index_ + 1, _players.size() ) );
    }
public:
    player_db() = default;
    player_db( const player_db& ) = default;
    player_db( app& app_ ) : _app( &app_ ) {}
    player_db( player_db&& other_ ) { *this = std::move( other_ ); }
    ~player_db() = default;
    player_db& operator=( const player_db& ) = default;
    player_db& operator=( player_db&& other_ ) {
        _players = std::move( other_._players );
        _app = std::move( other_._app );
        return *this;
    }
    bool is_player_name_set( string_id id_ = 0 ) {
        if ( id_ < _players.size() ) {
            return !_players[id_].empty();
        }
        return false;
    }

    void set_player_name( const std::wstring& name_, string_id id_ = 0 );

    void remove_player_name( string_id id_ ) {
        if ( id_ < _players.size() ) {
            _players[id_].clear();
        }
    }

    std::wstring get_player_name( string_id id_ = 0 ) {
        if ( id_ < _players.size() ) {
            return _players[id_];
        }
        return std::wstring {};
    }

    bool is_player_in_db( const std::wstring& name_ ) {
        return end( _players )
            != std::find( begin( _players ), end( _players ), name_ );
    }

    string_id get_player_id( const std::wstring& name_ ) {
        auto ref = std::find( begin( _players ), end( _players ), name_ );
        if ( ref == end( _players ) ) {
            auto id = _players.size();
            set_player_name( name_, id );
            return id;
        }
        return std::distance( begin( _players ), ref );
    }
};

#ifdef POP_MAX
#pragma pop_macro("max")
#undef POP_MAX
#endif