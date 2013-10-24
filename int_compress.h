#pragma once

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

inline bool get_bit( const unsigned char* blob_, size_t at_ ) {
    return ( ( blob_[at_ / 8] >> ( at_ % 8 ) ) & 1 ) != 0;
}

inline void set_bit( unsigned char* blob_, size_t at_, unsigned char set_ ) {
    auto bit = 1 << at_;
    blob_[at_ / 8] = ( blob_[at_ / 8] & ( ~bit ) ) | ( ( -set_ ) & bit );
}

inline size_t store_bit( unsigned char* blob_, size_t bit_offset_, unsigned char bit_ ) {
    set_bit(blob_, bit_offset_, bit_);
    return bit_offset_ + 1;
}

inline size_t store_bits( unsigned char* blob_, size_t bit_offset_, unsigned char* src_, size_t bits_ ) {
    unsigned bit = 0;
    for ( ; bit < bits_; ++bit ) {
        bit_offset_ = store_bit(blob_, bit_offset_, get_bit(src_, bit));
    }
    return bit_offset_;
}

inline size_t bit_pack_int( unsigned char* blob_, size_t bit_offset_, bool value_ ) {
    return store_bit( blob_, bit_offset_, value_ ? 1 : 0 );
}

namespace detail {
    template<typename IntType>
    inline unsigned count_bits(IntType value_) {
        unsigned bits = 1;
        value_ >>= 1;
        while ( value_ ) {
            ++bits;
            value_ >>= 1;
        }
        return bits;
        // for some odd reaseon
        // for ( ; value_ >> bits; ++bits ) will cause infinite loops ....
    }
}
template<typename IntType>
inline unsigned count_bits(IntType value_) {
    // needs explicit cast to unsigned, or sign extending will ruin your day with a infinite loops...
    return detail::count_bits(static_cast<typename std::make_unsigned<IntType>::type>( value_ ));
}

template<size_t Value>
struct bits_to_store : std::integral_constant<size_t, 1 + bits_to_store<( Value >> 1 )>::value> {};

template<>
struct bits_to_store<0> : std::integral_constant<size_t, 1>
{};

template<>
struct bits_to_store<1> : std::integral_constant<size_t, 1>
{};

template<size_t Value>
struct masking_for_bits : std::integral_constant<size_t, ( ( 1 << Value ) - 1 )> {};

template<typename IntType>
inline size_t bit_pack_int( unsigned char* blob_, size_t bit_offset_, IntType value_ ) {
    auto bits = count_bits(value_) - 1;
    const auto max_bits = bits_to_store<sizeof( IntType ) * 8>::value;

    bit_offset_ = store_bits( blob_, bit_offset_, reinterpret_cast<unsigned char*>( &bits ), max_bits );
    bit_offset_ = store_bits( blob_, bit_offset_, reinterpret_cast<unsigned char*>( &value_ ), bits + 1 );

    return bit_offset_;
}

inline size_t read_bit( const unsigned char* blob_, size_t bit_offset_, unsigned char& value_ ) {
    value_ = get_bit(blob_, bit_offset_);
    return bit_offset_ + 1;
}

inline size_t read_bits( const unsigned char* blob_, size_t offset_, unsigned char* dst_, size_t bits_ ) {
    for ( size_t i = 0; i < bits_; ++i ) {
        unsigned char bit;
        offset_ = read_bit(blob_, offset_, bit);
        store_bit(dst_, i, bit);
    }
    return offset_;
}

inline size_t bit_unpack_int( const unsigned char* blob_, size_t bit_offset_, bool& value_ ) {
    unsigned char c;
    auto new_off = read_bit(blob_, bit_offset_, c);
    value_ = c == 1;
    return new_off;
}

template<typename IntType>
inline size_t bit_unpack_int( const unsigned char* blob_, size_t bit_offset_, IntType& value_ ) {
    unsigned char bits = 0;
    const auto max_bits = bits_to_store<sizeof( IntType ) * 8>::value;
    bit_offset_ = read_bits( blob_, bit_offset_, reinterpret_cast<unsigned char*>( &bits ), max_bits );
    bit_offset_ = read_bits( blob_, bit_offset_, reinterpret_cast<unsigned char*>( &value_ ), bits + 1 );
    return bit_offset_;
}

template<typename IntType>
inline unsigned char* pack_int( unsigned char* from_, unsigned char* to_, IntType value_ ) {
    do {
        *from_ = (unsigned char( value_ ) & 0x7F) | 0x80;
        value_ >>= 7;
        *from_ &= 0xFF >> ( value_ ? 0 : 1 );
        ++from_;
    } while ( value_ && from_ < to_ );
    return from_;
}

inline unsigned char* pack_int( unsigned char* from_, unsigned char* to_, bool value_ ) {
    *from_ = value_ ? 1 : 0;
    return from_ + 1;
}

template<typename IntType>
inline const unsigned char* unpack_int( const unsigned char* from_, const unsigned char* to_, IntType& value_ ) {
    value_ = 0;
    unsigned shift = 0;
    bool cont = true;
    while ( cont && from_ < to_ ) {
        value_ |= IntType( ( *from_ ) & 0x7F ) << shift;
        shift += 7;
        cont = ( ( *from_ ) & 0x80 ) != 0;
        ++from_;
    }
    return from_;
}

inline const unsigned char* unpack_int( const unsigned char* from_, const unsigned char* to_, bool& value_ ) {
    value_ = ( *from_ == 1 );
    return from_ + 1;
}