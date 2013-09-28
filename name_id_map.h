#pragma once

#include <vector>

#include "swtor_log_parser.h"

#pragma push_macro("max")
#undef max

class name_id_map {
    // array index is local, stored ids are server
    typedef std::vector<string_id>          string_id_vector;

    string_id_vector    _map;

public:
    string_id add(string_id server_name_) {
        size_t index = 0;
        for ( ; index < _map.size(); ++index ) {
            if ( _map[index] == 0xFFFFFFFFFFFFFFFF ) {
                _map[index] = server_name_;
                return index;
            }
        }
        _map.push_back(server_name_);
        return index;
    }

    void remove(string_id server_name_) {
        using std::find_if;
        using std::begin;
        using std::end;

        auto ref = find_if(begin(_map), end(_map), [=](string_id sid_) {return sid_ == server_name_; });
        if ( ref == end(_map) ) {
            throw std::runtime_error("invalid name id mapping!");
        }
        *ref = server_name_;
    }

    void add(string_id server_id_, string_id local_id_) {
        using std::max;

        _map.resize(max(local_id_ + 1, _map.size()), 0xFFFFFFFFFFFFFFFF);
        if ( _map[local_id_] != 0xFFFFFFFFFFFFFFFF ) {
            throw std::runtime_error("name_id_map::add(server_id_, local_id_): local_id_ alleready in use!");
        }
        _map[local_id_] = server_id_;
    }

    string_id map_to_server(string_id local_) {
        return _map[local_];
    }

    string_id map_to_local(string_id server_) {
        using std::find_if;
        using std::begin;
        using std::end;
        using std::distance;

        auto ref = find_if(begin(_map), end(_map), [=](string_id sid_) {return sid_ == server_; });
        if ( ref == end(_map) ) {
            return server_;
        }

        return distance(begin(_map), ref);
    }
};

#pragma pop_macro("max");