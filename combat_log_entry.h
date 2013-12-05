#pragma once

#include <chrono>

typedef unsigned long long string_id;
typedef unsigned long long entity_id;

struct combat_log_entry {
    std::chrono::system_clock::time_point   time_index;
    string_id                               src;
    string_id                               src_minion;
    string_id                               dst;
    string_id                               dst_minion;
    string_id                               ability;
    string_id                               effect_action;
    string_id                               effect_type;
    string_id                               effect_value_type;
    string_id                               effect_value_type2;
    entity_id                               dst_id;
    entity_id                               src_id;
    int                                     effect_value;
    int                                     effect_value2;
    int                                     effect_thread;
    int                                     entry_flags;
};

enum log_entry_flags {
    effect_was_crit = 1 << 0,
    effect_was_crit2 = 1 << 1,
};