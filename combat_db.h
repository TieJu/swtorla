#pragma once

#include <vector>

#include "combat_log_entry.h"
#include "encounter.h"
#if 0
class cencounter {
    std::vector<combat_log_entry>           _cle;
    size_t                                  _begins { 0 };
    size_t                                  _ends { 0 };
    std::chrono::system_clock::time_point   _last;

    bool filter( const combat_log_entry& cle_ ) { return false; }
    static bool is_combat_begin( const combat_log_entry& cle_ ) {
        return cle_.effect_action == ssc_Event
            && cle_.effect_type == ssc_EnterCombat;
    }
    static bool is_combat_end( const combat_log_entry& cle_ ) {
        return cle_.effect_action == ssc_Event
            && cle_.effect_type == ssc_ExitCombat;
    }

public:
    void remap_player( string_id old_, string_id new_ ) {
        for ( auto& e : _cle ) {
            if ( e.src == old_ ) {
                e.src = new_;
            }
            if ( e.dst == old_ ) {
                e.dst = new_;
            }
        }
    }
    void on_combat_event( const combat_log_entry& cle_ ) {
        _last = cle_.time_index;
        if ( filter( cle_ ) ) {
            return;
        }

        _begins += is_combat_begin( cle_ )?1:0;
        _ends += is_combat_end( cle_ )?1:0;
        _cle.push_back( cle_ );
    }

    bool combat_has_finished() {
        if ( _begins > 0 ) {
            return _begins == _ends;
        }
        return false;
    }

    std::chrono::system_clock::time_point timestamp( ) { return _last; }
    
    template<typename DstType, typename U>
    query_set<std::vector<combat_log_entry>, DstType> select( U v_ ) {
        return select_from<DstType>( std::forward<U>( v_ ), _table );
    }
};
#endif
class combat_db {
    std::vector<encounter>      _encounters;

public:
    void remap_player( string_id old_, string_id new_ ) {
        for ( auto& enc : _encounters ) {
            enc.remap_player( old_, new_ );
        }
    }
    void on_combat_event( const combat_log_entry& cle_ ) {
        if ( _encounters.empty() ) {
            _encounters.resize( 1 );
        } else if ( _encounters.back().combat_has_finished() ) {
            _encounters.back().compatct();
            _encounters.resize( 1 + _encounters.size() );
        }
        _encounters.back().insert( cle_ );
    }
    void clear() {
        _encounters.clear();
    }
    size_t count_encounters( ) const { return _encounters.size( ); }
    encounter& from( size_t index_ ) { return _encounters[index_]; }
};