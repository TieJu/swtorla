#pragma once

#include <string>
#include <tuple>
#include <algorithm>
#include <unordered_map>
#include <vector>
#include <array>
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

inline std::wstring to_wstring( const combat_log_entry& e_ ) {
    std::wstring buffer;

    buffer += L"CombatLogEntry {";
    buffer += L" src: " + std::to_wstring( e_.src );
    buffer += L" src minion: " + std::to_wstring( e_.src_minion );
    buffer += L" src id: " + std::to_wstring( e_.src_id );
    buffer += L" dst: " + std::to_wstring( e_.dst );
    buffer += L" dst minion: " + std::to_wstring( e_.src_minion );
    buffer += L" src id: " + std::to_wstring( e_.src_id );
    buffer += L" effect acrtion: " + std::to_wstring( e_.effect_action );
    buffer += L" effect type: " + std::to_wstring( e_.effect_value_type );
    buffer += L" effect type2: " + std::to_wstring( e_.effect_value_type2 );
    buffer += L" value: " + std::to_wstring( e_.effect_value );
    buffer += L" value2: " + std::to_wstring( e_.effect_value2 );
    buffer += L" thread: " + std::to_wstring( e_.effect_thread );
    buffer += L" flags: " + std::to_wstring( e_.entry_flags );
    buffer += L" }";

    return buffer;
}

enum log_entry_flags {
    effect_was_crit = 1 << 0,
    effect_was_crit2 = 1 << 1,
};

typedef std::unordered_map<string_id, std::wstring>  string_to_id_string_map;
typedef std::vector<std::wstring>                    character_list;

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
    entry_flags,
    max_optional_elements
};

typedef std::tuple<std::array<unsigned char, sizeof(combat_log_entry) + 16 * sizeof(unsigned char)>, size_t>  compressed_combat_log_entry;

// returns uncompressed log entry and the number of bits read from buffer_
std::tuple<combat_log_entry, size_t> uncompress(const void* buffer_, size_t offset_bits_);
// returns compressed log entry an array and the number of bits used in that array
compressed_combat_log_entry compress( const combat_log_entry& e_ );

template<typename Iterator>
class combat_log_entry_compressed_wrap {
    Iterator  _from = Iterator {};
    Iterator  _to = Iterator {};

public:
    combat_log_entry_compressed_wrap() = default;
    template<typename U>
    combat_log_entry_compressed_wrap( U from_, U to_ )
        : _from( std::forward<U>(from_) )
        , _to( std::forward<U>(to_) ) {
    }
    combat_log_entry_compressed_wrap( const combat_log_entry_compressed_wrap& other_ ) = default;
    combat_log_entry_compressed_wrap& operator=( const combat_log_entry_compressed_wrap& other_ ) = default;

    operator combat_log_entry() const {
        return std::get<0>( uncompress( data(), 0 ) );
    }

    size_t size() const {
        return _to - _from;
    }

    Iterator data() const {
        return _from;
    }

    Iterator data() {
        return _from;
    }
};

template<typename U>
inline combat_log_entry_compressed_wrap<U> wrap_combat_log_entry_compressed( U from_, U to_ ) {
    return combat_log_entry_compressed_wrap<U>{from_, to_};
}

class combat_log_entry_compressed {
    std::array<unsigned char, sizeof(combat_log_entry)+16 * sizeof( unsigned char )>    _buffer;
    size_t                                                                              _length;

public:
    size_t size() const {
        return _length;
    }

    const unsigned char* data() const {
        return _buffer.data();
    }

    unsigned char* data() {
        return _buffer.data();
    }

    combat_log_entry_compressed& operator=( const combat_log_entry_compressed& other_ ) = default;

    operator combat_log_entry() const {
        return std::get<0>( uncompress( data(), 0 ) );
    }

    combat_log_entry_compressed() = default;
    combat_log_entry_compressed( const void* data_, size_t length_ ) {
        if ( length_ > _buffer.size() ) {
            throw std::runtime_error( "buffer data is too large for a compressed log entry" );
        }
        auto begin = reinterpret_cast<const unsigned char*>( data_ );
        auto end = begin + length_;
        std::copy( begin, end, _buffer.begin() );
    }
    combat_log_entry_compressed( const combat_log_entry_compressed& other_ ) = default;
    combat_log_entry_compressed( const combat_log_entry& other_ ) {
        auto re = compress( other_ );
        _buffer = std::get<0>( re );
        _length = ( std::get<1>( re ) +7 ) / 8;
    }
};

enum swtor_string_constants : unsigned long long {
    ssc_Event = 836045448945472ull,
    ssc_EnterCombat = 836045448945489ull,
    ssc_ExitCombat = 836045448945490ull,
    ssc_ApplyEffect = 836045448945477ull,
    ssc_Damage = 836045448945501ull,
    ssc_Heal = 836045448945500ull,
};