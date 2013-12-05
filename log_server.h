#pragma once

#include "net_protocol.h"

class log_server
: public net_protocol_server_interface {
    std::vector<std::unique_ptr<net_protocol>>  _client_links;
    std::unique_ptr<net_protocol>               _server_port;
    player_db                              _client_list;
    string_db                     _string_map;

public:
    log_server();
    virtual ~log_server() {}

    virtual bool on_client_register(net_protocol* client_, const wchar_t* name_start_, const wchar_t* name_end_) {
        auto at = std::find(begin(_client_links), end(_client_links), client_);
        if ( at == end(_client_links) ) {
            return false;
        }

        auto id = std::distance(begin(_client_links), at);
        _client_list[id].assign(name_start_, name_end_);

        for ( const auto& cl : _client_links ) {
            cl->broadcast_names(_client_list);
        }
        return true;
    }
    virtual bool on_client_unregister(net_protocol* client_, string_id ident_) {
        auto at = std::find(begin(_client_links), end(_client_links), client_);
        if ( at == end(_client_links) ) {
            return false;
        }
        
        auto id = std::distance(begin(_client_links), at);
        _client_list[id].clear();

        for ( const auto& cl : _client_links ) {
            cl->broadcast_names(_client_list);
        }
        return true;
    }
    virtual bool on_string_lookup(net_protocol* client_, string_id ident_) {
        auto at = _string_map.find(ident_);
        if ( at != end(_string_map) ) {
            client_->send_string_info(ident_, at->second);
            return true;
        }

        for ( const auto& cl : _client_links ) {
            cl->request_string_resolve(ident_);
        }
        return true;
    }
    virtual bool on_string_info(net_protocol* client_, string_id ident_, const wchar_t* string_start_, const wchar_t* string_end_) {
        if ( _string_map.count(ident_) ) {
            return true;
        }

        auto& target = _string_map[ident_];
        target.assign(string_start_, string_end_);

        for ( const auto& cl : _client_links ) {
            if ( cl.get() != client_ ) {
                cl->send_string_info(ident_, target);
            }
        }
        return true;
    }

    virtual bool on_combat_event(net_protocol* client_, const combat_log_entry& e_) {
        for ( const auto& cl : _client_links ) {
            if ( cl.get() != client_ ) {
                cl->send_combat_event(e_);
            }
        }
        return true;
    }
};