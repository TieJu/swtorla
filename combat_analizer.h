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
                auto records = _encounters.back().select<combat_log_entry>([=](const combat_log_entry& e_) {
                    return e_.src == 0 && e_.src_minion == string_id(-1);
                }, [](const combat_log_entry& e_) {
                    return e_;
                }, [](const combat_log_entry&, const combat_log_entry&) {
                    return false;
                }, [](combat_log_entry&, const combat_log_entry&) {
                });
                int dmg = 0;
                int heal = 0;
                records.for_each([&](const combat_log_entry& e_) {
                    if ( e_.effect_action == ssc_ApplyEffect ) {
                        if ( e_.effect_type == ssc_Damage ) {
                            dmg += e_.effect_value;
                        } else if ( e_.effect_type == ssc_Heal ) {
                            heal += e_.effect_value;
                        }
                    }
                });
                BOOST_LOG_TRIVIAL(debug) << L"encounter ended:";
                BOOST_LOG_TRIVIAL(debug) << L"char 0 did " << dmg << L" dmg";
                BOOST_LOG_TRIVIAL(debug) << L"char 0 did " << heal << L" healing";

                auto per_skill = records.select<combat_log_entry>([=](const combat_log_entry& e_) {
                    return true;
                }, [](const combat_log_entry& e_) {
                    return e_;
                }, [](const combat_log_entry& lhs_, const combat_log_entry& rhs_) {
                    return lhs_.ability == rhs_.ability;
                }, [](combat_log_entry& lhs_, const combat_log_entry& rhs_) {
                    lhs_.effect_value += rhs_.effect_value;
                    lhs_.effect_value2 += rhs_.effect_value2;
                    lhs_.effect_thread += rhs_.effect_thread;
                });
                per_skill.for_each([=](const combat_log_entry& e_) {
                    BOOST_LOG_TRIVIAL(debug) << L"skill " << e_.ability << " did " << e_.effect_value << " dmg / healing";
                });
                return;
            }
        }
        if ( _record ) {
            _encounters.back().insert(e_);
        }
    }

    template<typename R, typename U, typename V, typename W, typename X>
    query_result<R> select_from(size_t encounter_id_,U filter_, V transfrom_, W same_group_, X group_combine_) {
        return _encounters[encounter_id_].select<R>(std::forward<U>( filter_ ), std::forward<V>( transform_ ), std::forward<W>( same_group_ ), std::forward<X>( group_combine_ ));
    }

    size_t count_encounters() {
        return _encounters.size();
    }
};