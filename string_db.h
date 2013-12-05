#pragma once

#include <string>
#include <unordered_map>

#include "combat_log_entry.h"

class app;
class string_db {
    std::unordered_map<string_id, std::wstring> _map;
    app*                                        _app { nullptr };

public:
    string_db() = default;
    string_db( const string_db& ) = default;
    string_db( string_db&& other_ ) { *this = std::move( other_ ); }
    string_db( app& app_ ) : _app( &app_ ) {}
    ~string_db() = default;
    string_db& operator=( const string_db& ) = default;
    string_db& operator=( string_db&& other_ ) {
        _map = std::move( other_._map );
        _app = std::move( other_._app );
        return *this;
    }

    bool is_set( string_id id_ ) {
        return end( _map ) != _map.find( id_ );
    }

    std::wstring get( string_id id_ );

    void set( string_id id_, std::wstring str_ ) {
        _map.insert( std::make_pair( id_, std::move( str_ ) ) );
    }
};