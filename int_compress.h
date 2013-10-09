#pragma once

inline bool get_bit( unsigned const char* blob_, size_t at_ ) {
    return ( ( blob_[at_ / 8] >> ( at_ % 8 ) ) & 1 ) != 0;
}

inline void set_bit( unsigned char* blob_, size_t at_, unsigned char set_ ) {
    /*if ( set_ ) {
        blob_[at_ / 8] |= 1 << ( at_ % 8 );
    } else {
        blob_[at_ / 8] &= ~( 1U << ( at_ % 8 ) );
    }*/
    auto bit = 1 << at_;
    blob_[at_ / 8] = ( blob_[at_ / 8] & ~bit ) | ( -set_ & bit );
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
    return store_bit(blob_, bit_offset_, value_);
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

    bit_offset_ = store_bits(blob_, bit_offset_, reinterpret_cast<unsigned char*>( &bits ), max_bits);
    bit_offset_ = store_bits( blob_, bit_offset_, reinterpret_cast<unsigned char*>( &value_ ), bits + 1 );

    return bit_offset_;
}

inline size_t read_bit( const unsigned char* blob_, size_t bit_offset_, bool& value_ ) {
    value_ = get_bit(blob_, bit_offset_);
    return bit_offset_ + 1;
}

inline size_t read_bits( const unsigned char* blob_, size_t offset_, unsigned char* dst_, size_t bits_ ) {
    for ( size_t i = 0; i < bits_; ++i ) {
        bool bit;
        offset_ = read_bit(blob_, offset_, bit);
        store_bit(dst_, i, bit);
    }
    return offset_;
}

inline size_t bit_unpack_int( const unsigned char* blob_, size_t bit_offset_, bool& value_ ) {
    return read_bit(blob_, bit_offset_, value_);
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
        *from_ = unsigned char( value_ & 0x7F );
        value_ >>= 7;
        if ( value_ ) {
            *from_ |= 0x80;
        }
        ++from_;
    } while ( value_ && from_ < to_ );
    return from_;
}

inline unsigned char* pack_int( unsigned char* from_, unsigned char* to_, unsigned char value_ ) {
    *from_ = value_ ? 1 : 0;
    return from_ + 1;
}

inline unsigned long long unpack_int( unsigned char* from_, unsigned char* to_ ) {
    unsigned long long value = 0;

    for ( size_t i = 0; from_ + i < to_; ++i ) {
        value &= ( from_[i] & 0x7F ) << ( 7 * i );
        if ( from_[i] & 0x80 ) {
            continue;
        }
        break;
    }

    return value;
}