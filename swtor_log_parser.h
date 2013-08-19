#pragma once

#include <string>
#include <tuple>
#include <algorithm>
#include <unordered_map>
#include <vector>
#include <array>
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
    string_id                   src_minion;
    unsigned long long          src_id;
    string_id                   dst;
    string_id                   dst_minion;
    unsigned long long          dst_id;
    string_id                   ability;
    string_id                   effect_action;
    string_id                   effect_type;
    int                         effect_value;
    bool                        was_crit_effect;
    string_id                   effect_value_type;
    int                         effect_value2;
    bool                        was_crit_effect2;
    string_id                   effect_value_type2;
    int                         effect_thread;
};

typedef concurrency::concurrent_unordered_map<string_id, std::wstring>  string_to_id_string_map;
typedef concurrency::concurrent_vector<std::wstring>                    character_list;

combat_log_entry parse_combat_log_line(const char* from_, const char* to_, string_to_id_string_map& string_map_, character_list& char_list_);

enum log_entry_optional_elements {
    src_minion,
    src_id,
    dst,            // if src = dst this is not set
    dst_minion,
    dst_id,
    effect_value_type,
    effect_value_2,
    effect_value_type2,
    effect_thread,
    max_optional_elements
};

// returns uncompressed log entry and the number of bits read from buffer_
std::tuple<combat_log_entry, size_t> uncompress(const void* buffer_, size_t offset_bits_);
// returns compressed log entry an array and the number of bits used in that array
std::tuple<std::array<char, sizeof( combat_log_entry ) + 3>, size_t> compress(const combat_log_entry& e_);