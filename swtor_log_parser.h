#pragma once

#include <string>
#include <tuple>
#include <algorithm>
#include <unordered_map>
#include <vector>
#include <concurrent_unordered_map.h>
#include <concurrent_vector.h>

typedef unsigned long long string_id;

struct combat_log_entry_time_index {
    unsigned int    hours;
    unsigned int    minutes;
    unsigned int    seconds;
    unsigned int    milseconds;
};
struct combat_log_entry {
    combat_log_entry_time_index time_index;
    string_id                   src;
    unsigned long long          src_id;
    string_id                   dst;
    unsigned long long          dst_id;
    string_id                   ability;
    string_id                   effect_action;
    string_id                   effect_type;
    unsigned int                effect_value;
    bool                        was_crit_effect;
    string_id                   effect_value_type;
    unsigned int                effect_thread;
};

typedef concurrency::concurrent_unordered_map<string_id, std::string>  string_to_id_string_map;
typedef concurrency::concurrent_vector<std::string>                    character_list;

combat_log_entry parse_combat_log_line(const char* from_, const char* to_, string_to_id_string_map& string_map_, character_list& char_list_);