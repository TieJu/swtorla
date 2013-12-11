#pragma once

#include <vector>

#include "combat_log_entry.h"
#include "encounter.h"

class combat_db {
    std::vector<encounter>      _encounters;
    
    static bool is_combat_begin( const combat_log_entry& cle_ ) {
        return cle_.effect_action == ssc_Event
            && cle_.effect_type == ssc_EnterCombat;
    }
    static bool is_combat_end( const combat_log_entry& cle_ ) {
        return cle_.effect_action == ssc_Event
            && cle_.effect_type == ssc_ExitCombat;
    }

    bool filter( const combat_log_entry& cle_ ) {
        if ( is_combat_begin( cle_ ) || is_combat_end( cle_ ) ) {
            return false;
        }
        if ( is_safe_login( cle_ ) ) {
            return true;
        }
        if ( is_heal_or_damabe_effect( cle_ ) ) {
            return false;
        }
        return true;
    }

public:
    void remap_player( string_id old_, string_id new_ ) {
        for ( auto& enc : _encounters ) {
            enc.remap_player( old_, new_ );
        }
    }
    bool on_combat_event( const combat_log_entry& cle_ ) {
        if ( filter( cle_ ) ) {
            return false;
        }
        if ( _encounters.empty() ) {
            _encounters.resize( 1 );
        } else if ( _encounters.back().combat_has_finished() ) {
            _encounters.back().compatct();
            _encounters.resize( 1 + _encounters.size() );
        }
        return _encounters.back().insert( cle_ );
    }
    void clear() {
        _encounters.clear();
    }
    size_t count_encounters( ) const { return _encounters.size( ); }
    encounter& from( size_t index_ ) { return _encounters[index_]; }
};