#include "app.h"

std::wstring string_db::get( string_id id_ ) {
    auto at = _map.find( id_ );
    if ( at != end( _map ) ) {
        return at->second;
    }
    
    _app->lookup_string( id_ );
    return L"missing translation for " + std::to_wstring( id_ );
}