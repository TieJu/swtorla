#include "swtor_log_parser.h"

static void expect_char(const char*& from_, const char* to_,char c_) {
    if ( from_ == to_ ) {
        throw std::runtime_error("unexpected end of log line");
    }

    if ( *from_++ != c_ ) {
        throw std::runtime_error("unexpected character found");
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
Target parse_number(const char*& from_, const char* to_) {
    Target t{};
    for ( ; from_ != to_ && is_number_char(*from_); ++from_, t *= 10 ) {
        t += *from_ - '0';
    }
    return t / 10;
}

static void set_pos(const char*& from_, const char* to_, const char *new_pos_) {
    if ( new_pos_ > to_ ) {
        throw std::runtime_error("malformed combat log line");
    }
    from_ = new_pos_;
}

static string_id register_string(string_id id, const char* from_, const char* to_, string_to_id_string_map& string_map_) {
    auto loc = string_map_.find(id);
    if ( loc == end(string_map_) ) {
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

    char_list_.emplace_back(from_, to_);

    return char_list_.size() - 1;
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
    expect_char(from_, to_, '[');
    auto end = find_char(from_,to_,']');
    if ( check_char(from_, to_, '@') ) {
        // todo: handle crew correctly
        e.src = register_name(from_, end, char_list_);
    } else {
        auto id_start = find_char(from_, end,'{');
        auto name_start = from_;
        // id_start - 1, because the name follows one space (todo: find a bether way to do it)
        auto name_end = id_start - 1;
        set_pos(from_, to_, id_start + 1);
        e.src = register_string(parse_number<decltype( e.src )>( from_, end ), name_start, name_end, string_map_);

        from_ = find_char(from_, to_, ':');
        set_pos(from_, to_, from_ + 1);
        skip_spaces(from_, to_);
        e.src_id = parse_number<decltype( e.src_id )>( from_, end );
    }
    // skip over ]
    set_pos(from_, to_, end + 1);

    skip_spaces(from_, to_);

    // destination block
    // format:
    // 1) '[''@'<player char name>']'
    // 2) '[''@'<player char name>':'<crew name>'{'<crew name id'}'']'
    // 3) '['<mob name> '{'<mob name id'}'':''{'<mob id>'}'']'
    expect_char(from_, to_, '[');
    end = find_char(from_, to_, ']');
    if ( check_char(from_, to_, '@') ) {
        // todo: handle crew correctly
        e.dst = register_name(from_, end, char_list_);
    } else {
        auto id_start = find_char(from_, end, '{');
        auto name_start = from_;
        // id_start - 1, because the name follows one space (todo: find a bether way to do it)
        auto name_end = id_start - 1;
        set_pos(from_, to_, id_start + 1);
        e.dst = register_string(parse_number<decltype( e.dst )>( from_, end ), name_start, name_end, string_map_);

        from_ = find_char(from_, to_, ':');
        set_pos(from_, to_, from_ + 1);
        skip_spaces(from_, to_);
        e.dst_id = parse_number<decltype( e.dst_id )>( from_, end );
    }
    // skip over ]
    set_pos(from_, to_, end + 1);

    skip_spaces(from_, to_);
    
    // ability block
    // format:
    // 1) '['<name> '{'<name id>'}'']'
    // 2) '['']'
    expect_char(from_, to_, '[');
    end = find_char(from_, to_, ']');
    if ( end - from_ > 0 ) {
        auto id_start = find_char(from_, to_, '{');
        auto name_start = from_;
        // id_start - 1, because the name follows one space (todo: find a bether way to do it)
        auto name_end = id_start - 1;
        set_pos(from_, to_, id_start + 1);
        e.ability = register_string(parse_number<decltype( e.ability )>( from_, end ), name_start, name_end, string_map_);
    }
    // skip over ]
    set_pos(from_, to_, end + 1);

    skip_spaces(from_, to_);

    // effect block
    // format:
    // '['<effect action name> '{'<effect action name id>'}'':' <effect type> '{'<effect type name>'}'']'
    expect_char(from_, to_, '[');
    end = find_char(from_, to_, ']');
    auto id_start = find_char(from_, to_, '{');
    auto name_start = from_;
    // id_start - 1, because the name follows one space (todo: find a bether way to do it)
    auto name_end = id_start - 1;
    set_pos(from_, to_, id_start + 1);
    e.effect_action = register_string(parse_number<decltype( e.effect_action )>( from_, end ), name_start, name_end, string_map_);

    from_ = find_char(from_, to_, ':') + 1;
    skip_spaces(from_, to_);

    id_start = find_char(from_, to_, '{');
    name_start = from_;
    // id_start - 1, because the name follows one space (todo: find a bether way to do it)
    name_end = id_start - 1;
    set_pos(from_, to_, id_start + 1);
    e.effect_type = register_string(parse_number<decltype( e.effect_type )>( from_, end ), name_start, name_end, string_map_);
    // skip over ]
    set_pos(from_, to_, end + 1);

    skip_spaces(from_, to_);

    // effect value block
    // format:
    // 1) '('<number>')'
    // 2) '('<numver>'*'')' -> crit
    // 3) '('<number>' <dmg type name> '{'<dmg type name id>'}')'
    // 4) '('<number>'* <dmg type name> '{'<dmg type name id>'}')' -> crit

    expect_char(from_, to_, '(');
    e.effect_value = parse_number<decltype( e.effect_value )>( from_, to_ );
    if ( check_char(from_,to_,'*') ) {
        e.was_crit_effect = true;
    }
    if ( !check_char(from_, to_, ')') ) {
        skip_spaces(from_, to_);
        id_start = find_char(from_, to_, '{');
        name_start = from_;
        // id_start - 1, because the name follows one space (todo: find a bether way to do it)
        name_end = id_start - 1;
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