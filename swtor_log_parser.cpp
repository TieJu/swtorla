#include "swtor_log_parser.h"

#include <type_traits>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

static void expect_char(const char*& from_, const char* to_,char c_) {
    if ( from_ == to_ ) {
        throw std::runtime_error("unexpected end of log line");
    }

    if ( *from_++ != c_ ) {
        throw std::runtime_error("unexpected character found, expected " + std::string(1,c_) + " but got " + std::string((from_ - 1),to_));
    }
}

static bool check_char(const char*& from_, const char* to_, char c_) {
    if ( from_ == to_ ) {
        return false;
    }

    if ( *from_ == c_ ) {
        ++from_;
        return true;
    }

    return false;
}

template<size_t N>
static bool check_string(const char*& from_, const char* to_, const char (&str_)[N]) {
    auto cpy = from_;
    for ( size_t n = 0; n < N - 1; ++n ) {
        if ( *from_++ != str_[n] ) {
            from_ = cpy;
            return false;
        }
    }
    return true;
}

static void skip_spaces(const char*& from_, const char* to_) {
    from_ = std::find_if(from_, to_, [](char c_) { return c_ != ' '; });
}

static bool is_number_char(char c_) {
    return c_ >= '0'
        && c_ <= '9';
}

static const char* find_char(const char* from_, const char* to_, char c_) {
    return std::find(from_, to_, c_);
}

template<typename Target>
typename std::enable_if<std::is_signed<Target>::value, Target>::type parse_number(const char*& from_, const char* to_) {
    Target t
    {};
    bool negate = check_char(from_, to_, '-');
    for ( ; from_ != to_ && is_number_char(*from_); ++from_, t *= 10 ) {
        t += *from_ - '0';
    }
    return ( negate ? -t : t ) / 10;
}

template<typename Target>
typename std::enable_if<!std::is_signed<Target>::value, Target>::type parse_number(const char*& from_, const char* to_) {
    Target t{};
    for ( ; from_ != to_ && is_number_char(*from_); ++from_, t *= 10 ) {
        t += *from_ - '0';
    }
    return t / 10;
}

static void set_pos(const char*& from_, const char* to_, const char *new_pos_) {
    if ( new_pos_ > to_ ) {
        throw std::runtime_error("malformed combat log line, tryed to skip over line end, current pos is: " + std::string(from_,to_));
    }
    from_ = new_pos_;
}

static string_id register_string(string_id id, const char* from_, const char* to_, string_to_id_string_map& string_map_) {
    auto loc = string_map_.find(id);
    if ( loc == end(string_map_) ) {
        //BOOST_LOG_TRIVIAL(debug) << "new string " << std::string(from_, to_);
        string_map_[id].assign(from_, to_);
    }
    return id;
}

static string_id register_name(const char* from_, const char* to_, character_list& char_list_) {
    const auto len = to_ - from_;
    auto loc = std::find_if(begin(char_list_), end(char_list_), [=](const std::string& name_) {
        return !name_.compare(0, std::string::npos, from_, len);
    });

    if ( loc != end(char_list_) ) {
        return std::distance(begin(char_list_), loc);
    }

    char_list_.push_back(std::string(from_, to_));

    return char_list_.size() - 1;
}

static std::tuple<string_id, string_id, unsigned long long> read_entity_name(const char*& from_, const char* to_, string_to_id_string_map& string_map_, character_list& char_list_) {
    expect_char(from_, to_, '[');
    if ( check_char(from_, to_, ']') ) {
        return std::make_tuple(string_id(-1), string_id(-1), ( unsigned long long )(-1));
    }
    auto start = from_;
    size_t level = 1;
    for ( ; from_ != to_ && level; ++from_ ) {
        if ( *from_ == '[' ) {
            ++level;
        } else if ( *from_ == ']' ) {
            --level;
        }
    }

    if ( check_char(start, from_, '@') ) {
        auto sep = find_char(start, from_, ':');
        if ( sep != from_ ) {
            expect_char(sep, from_, ':');
            auto owner = register_name(start, sep - 1, char_list_);
            auto id_start = find_char(sep, to_, '{');
            expect_char(id_start, from_, '{');
            auto minion = register_string(parse_number<string_id>( id_start, from_ ), sep, id_start - 1, string_map_);
            return std::make_tuple(owner, minion, ( unsigned long long )( -1 ));
        } else {
            auto owner = register_name(start, from_ - 1, char_list_);
            return std::make_tuple(owner, string_id(-1), ( unsigned long long )( -1 ));
        }
    } else {
        //BOOST_LOG_TRIVIAL(debug) << std::string(start, to_);
        auto id_start = find_char(start, to_, '{');
        //BOOST_LOG_TRIVIAL(debug) << std::string(id_start, to_);
        expect_char(id_start, from_, '{');
        //BOOST_LOG_TRIVIAL(debug) << std::string(id_start, from_);
        //BOOST_LOG_TRIVIAL(debug) << std::string(start, id_start - 2);
        //BOOST_LOG_TRIVIAL(debug) << "number: " << std::string(id_start, from_);
        auto string_end = id_start - 2;// if this is put as param instead into register_string, strange things happens
        //BOOST_LOG_TRIVIAL(debug) << "name: " << std::string(start, id_start - 2);
        auto sid = parse_number<string_id>( id_start, from_ );
        auto mob = register_string(sid, start, string_end, string_map_);
        expect_char(id_start, from_, '}');
        skip_spaces(id_start, from_);
        expect_char(id_start, from_, ':');
        //BOOST_LOG_TRIVIAL(debug) << std::string(id_start, from_);
        auto id = parse_number<unsigned long long>( id_start, from_ );
        return std::make_tuple(mob, string_id(-1), id);
    }
}

template<typename CharCheck>
static string_id read_localized_string(const char*& from_, const char* to_, string_to_id_string_map& string_map_, CharCheck char_check_, char start_char_ = '[', char end_char_ = ']') {
    expect_char(from_, to_, start_char_);
    skip_spaces(from_, to_);
    if ( check_char(from_, to_, end_char_) ) {
        return string_id(-1);
    }

    // localized strings can contain [ and ] ....
    auto start = from_;
    for ( ; from_ != to_; ++from_ ) {
        if ( !char_check_(*from_) ) {
            break;
        }
    }
    expect_char(from_, to_, end_char_);

    auto id_start = find_char(start, from_, '{');
    expect_char(id_start, from_, '{');
    auto name_end = id_start - 1;
    auto id = parse_number<string_id>(id_start, from_);
    return register_string(id, start, name_end, string_map_);
}

combat_log_entry parse_combat_log_line(const char* from_, const char* to_, string_to_id_string_map& string_map_, character_list& char_list_) {
    using std::find_if;
    combat_log_entry e
    {};

    skip_spaces(from_, to_);

    // timestamp block
    // format: '['HH':'MM':'SS'.'lll']'
    expect_char(from_, to_, '[');
    e.time_index.hours = parse_number<decltype( e.time_index.hours )>( from_, to_ );
    expect_char(from_, to_, ':');
    e.time_index.minutes = parse_number<decltype( e.time_index.minutes )>( from_, to_ );
    expect_char(from_, to_, ':');
    e.time_index.seconds = parse_number<decltype( e.time_index.seconds )>( from_, to_ );
    expect_char(from_, to_, '.');
    e.time_index.milseconds = parse_number<decltype( e.time_index.milseconds )>( from_, to_ );
    expect_char(from_, to_, ']');

    skip_spaces(from_, to_);

    // source block
    // format:
    // 1) '[''@'<player char name>']'
    // 2) '[''@'<player char name>':'<crew name>'{'<crew name id'}'']'
    // 3) '['<mob name> '{'<mob name id'}'':''{'<mob id>'}'']'
    std::tie(e.src, e.src_minion, e.src_id) = read_entity_name(from_, to_, string_map_, char_list_);

    skip_spaces(from_, to_);

    // destination block
    // format:
    // 1) '[''@'<player char name>']'
    // 2) '[''@'<player char name>':'<crew name>'{'<crew name id'}'']'
    // 3) '['<mob name> '{'<mob name id'}'':''{'<mob id>'}'']'
    std::tie(e.dst, e.dst_minion, e.dst_id) = read_entity_name(from_, to_, string_map_, char_list_);

    skip_spaces(from_, to_);
    
    // ability block
    // format:
    // 1) '['<name> '{'<name id>'}'']'
    // 2) '['']'
    size_t level = 1;
    e.ability = read_localized_string(from_, to_, string_map_, [&](char c_) {
        if ( c_ == '[' ) {
            ++level;
        } else if ( c_ == ']' ) {
            --level;
            return level != 0;
        }
        return true;
    });

    skip_spaces(from_, to_);

    // effect block
    // format:
    // '['<effect action name> '{'<effect action name id>'}'':' <effect type> '{'<effect type name>'}'']'
    e.effect_action = read_localized_string(from_, to_, string_map_, [](char c_) {
        return c_ != ':';
    }, '[', ':');
    --from_;
    level = 1;
    e.effect_type = read_localized_string(from_, to_, string_map_, [&](char c_) {
        if ( c_ == '[' ) {
            ++level;
        } else if ( c_ == ']' ) {
            --level;
            return level != 0;
        }
        return true;
    },':');

    skip_spaces(from_, to_);

    // effect value block
    // format:
    // 1) '('<number>')'
    // 2) '('<numver>'*'')' -> crit
    // 3) '('<number>' <dmg type name> '{'<dmg type name id>'}')'
    // 4) '('<number>'* <dmg type name> '{'<dmg type name id>'}')' -> crit
    expect_char(from_, to_, '(');
    e.effect_value = parse_number<decltype( e.effect_value )>( from_, to_ );
    if ( check_char(from_, to_, '*') ) {
        e.was_crit_effect = true;
    }
    skip_spaces(from_, to_);
    if ( check_string(from_, to_, "-)") ) {
        // skip over it
    } else if ( !check_char(from_, to_, ')') ) {
        skip_spaces(from_, to_);
        auto id_start = find_char(from_, to_, '{');
        auto name_start = from_;
        // id_start - 1, because the name follows one space (todo: find a bether way to do it)
        auto name_end = id_start - 1;
        set_pos(from_, to_, id_start + 1);
        e.effect_value_type = register_string(parse_number<decltype( e.effect_value_type )>( from_, to_ ), name_start, name_end, string_map_);
        from_ = find_char(from_, to_, ')');
        // skip over )
        set_pos(from_, to_, from_ + 1);
    }

    skip_spaces(from_, to_);

    // thread block (optional)
    // format:
    // '<'<thread value>'>'
    if ( check_char(from_, to_, '<') ) {
        e.effect_thread = parse_number<decltype( e.effect_thread )>( from_, to_ );
        expect_char(from_, to_, '>');
    }

    return e;
}