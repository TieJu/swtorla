#pragma once

#include <string>
#include <tuple>
#include <algorithm>
#include <unordered_map>
#include <vector>
#include <array>
#include <chrono>
#include <concurrent_unordered_map.h>
#include <concurrent_vector.h>

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
    bool                                    was_crit_effect;
    bool                                    was_crit_effect2;
};

typedef concurrency::concurrent_unordered_map<string_id, std::wstring>  string_to_id_string_map;
typedef concurrency::concurrent_vector<std::wstring>                    character_list;

combat_log_entry parse_combat_log_line( const char* from_, const char* to_, string_to_id_string_map& string_map_, character_list& char_list_, std::chrono::system_clock::time_point time_offset );

enum log_entry_optional_elements {
    src_minion,
    src_id,
    dst,            // if src = dst this is not set
    dst_minion,
    dst_id,
    effect_value_type,
    effect_value2,
    effect_value_type2,
    effect_thread,
    effect_value,
    max_optional_elements
};

typedef std::tuple<std::array<unsigned char, sizeof(combat_log_entry) * 2>, size_t>  compressed_combat_log_entry;

// returns uncompressed log entry and the number of bits read from buffer_
std::tuple<combat_log_entry, size_t> uncompress(const void* buffer_, size_t offset_bits_);
// returns compressed log entry an array and the number of bits used in that array
compressed_combat_log_entry compress( const combat_log_entry& e_ );