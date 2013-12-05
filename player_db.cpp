#include "app.h"

void player_db::set_player_name( const std::wstring& name_, string_id id_ ) {
    ensure_index_is_present( id_ );
    if ( is_player_in_db( name_ ) ) {
        auto old = get_player_id( name_ );
        if ( old != id_ ) {
            _app->remap_player_name( old, id_ );
            remove_player_name( old );
        }
    }
    _players[id_] = name_;
}