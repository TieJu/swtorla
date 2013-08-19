#pragma once

#include "swtor_log_parser.h"
#include "encounter.h"

#include <concurrent_vector.h>

enum swtor_string_constants : unsigned long long {
    ssc_Event = 836045448945472ull,
    ssc_EnterCombat = 836045448945489ull,
    ssc_ExitCombat = 836045448945490ull,
    ssc_ApplyEffect = 836045448945477ull,
    ssc_Damage = 836045448945501ull,
    ssc_Heal = 836045448945500ull,
};

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
    concurrency::concurrent_vector<encounter>   _encounters;
    bool                                        _record;


public:
    combat_analizer() : _record(false) {}
    void add_entry(const combat_log_entry& e_) {
        if ( e_.effect_action == ssc_Event ) {
            if ( e_.effect_type == ssc_EnterCombat ) {
                _encounters.push_back(encounter{});
                _record = true;
                return;
            } else if ( e_.effect_type == ssc_ExitCombat ) {
                _record = false;
                return;
            }
        }
        if ( _record ) {
            _encounters.back().insert(e_);
        }
    }

    template<typename DstType, typename U>
    query_set<concurrency::concurrent_vector<combat_log_entry>, DstType> select_from(size_t index_,U v_) {
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