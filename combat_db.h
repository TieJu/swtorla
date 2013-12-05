#pragma once

#include <vector>

#include "combat_log_entry.h"

class cencounter {
    std::vector<combat_log_entry>   _cle;

public:
    void remap_player( string_id old_, string_id new_ );
};

class combat_db {
    std::vector<cencounter>  _encounters;

public:
    void remap_player( string_id old_, string_id new_ ) {
        for ( auto& enc : _encounters ) {
            enc.remap_player( old_, new_ );
        }
    }
    void on_combat_event( const combat_log_entry& cle_ ) {}
};