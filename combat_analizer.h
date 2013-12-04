#pragma once

#include "swtor_log_parser.h"
#include "encounter.h"

#include <vector>

class app;

struct combat_log_entry_ex : combat_log_entry {
    int     hits;
    int     crits;
    int     misses;

    combat_log_entry_ex() {}
    combat_log_entry_ex(const combat_log_entry& e_) {
        memcpy(this, &e_, sizeof( e_ ));
    }
};

class combat_analizer {
    std::vector<encounter>  _encounters;
    bool                    _record = false;
    app*                    _cl = nullptr;

public:
    combat_analizer() = default;
    explicit combat_analizer( app* app_ ) : _cl( app_ ) {}
    combat_analizer( const combat_analizer& ) = default;
    combat_analizer& operator=(const combat_analizer&) = default;
    combat_analizer( combat_analizer&& other_ ) : combat_analizer() { *this = std::move( other_ ); }
    combat_analizer& operator=( const combat_analizer&& other_ ) {
        _encounters = std::move( other_._encounters );
        _record = std::move( other_._record );
        _cl = std::move( other_._cl );
        return *this;
    }
    void add_entry( const combat_log_entry& e_ );
    void operator()( const combat_log_entry& e_ ) { add_entry( e_ ); }

    template<typename DstType, typename U>
    query_set<std::vector<combat_log_entry>, DstType> select_from(size_t index_,U v_) {
        return _encounters[index_].select<DstType>( std::forward<U>( v_ ) );
    }

    encounter& from(size_t index_) {
        return _encounters[index_];
    }

    size_t count_encounters() {
        return _encounters.size();
    }

    void clear() {
        _record = false;
        _encounters.clear();
    }
};