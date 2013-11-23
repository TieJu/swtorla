#include "app.h"
#include "combat_analizer.h"

void combat_analizer::add_entry( const combat_log_entry& e_ ) {
    if ( e_.effect_action == ssc_Event ) {
        if ( e_.effect_type == ssc_EnterCombat ) {
            _encounters.push_back( encounter {} );
            _record = true;
            _cl->player_change( e_.src );
            return;
        } else if ( e_.effect_type == ssc_ExitCombat ) {
            _record = false;
            return;
        }
    }
    if ( _record ) {
        _encounters.back().insert( e_ );
    }
}