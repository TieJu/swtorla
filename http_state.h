#pragma once

#include <vector>
#include <iterator>
#include <unordered_map>
#include <tuple>

template<typename Stream>
struct content_store_stream {
    Stream              stream;
    size_t              limit;
    template<typename Iterator>
    void store(Iterator from_, Iterator to_) {
        for ( ; from_ != to_ && limit; ++from_, --limit ) {
            stream.put(*from_);
        }
    }
    void limit_length_to(size_t l) {
        limit = l;
    }
    content_store_stream() : limit(size_t(-1)) {}
    content_store_stream(content_store_stream && o_) : stream(std::move(o_.stream)), limit(o_.limit) {}
    template<typename U>
    content_store_stream(U v_) : stream(std::forward<U>( v_ )), limit(size_t(-1)) {}
};

template<typename ContentStore>
class http_state {
    enum class state {
        http_code,
        read_header,
        store_content,
        finished,
        error,
    };
    enum class content_mode {
        copy,
        chunked,
    };
    state                                       _state;
    content_mode                                _content_mode;
    unsigned                                    _http_result;
    ContentStore                                _content;
    std::unordered_map<std::string, std::string>_header_values;

    template<typename Iterator>
    std::tuple<Iterator, size_t> parse_chunk_header(Iterator from_, Iterator to_) {
        auto start = from_;
        auto last = from_;
        for ( ; from_ != to_; ++from_ ) {
            if ( *last == '\r' ) {
                if ( *from_ == '\n' ) {
                    ++from_;
                    break;
                }
            }

            last = from_;
        }

        if ( from_ == to_ ) {
            _state = state::error;
            return std::make_tuple(to_, size_t(0));
        }

        size_t len = std::stoull(std::string(start, from_ - 2), nullptr, 16);

        return std::make_tuple(from_,len);

    }

    template<typename Iterator>
    void copy_data(Iterator from_, Iterator to_) {
        switch ( _content_mode ) {
        case content_mode::copy:
            _content.store(from_, to_);
            break;
        case content_mode::chunked: {
                size_t len;
                std::tie(from_, len) = parse_chunk_header(from_, to_);
                if ( len == 0 ) {
                    _state = state::finished;
                } else {
                    _content.store(from_, from_ + len);
                }
            }
            break;
        }
    }

    template<typename Iterator>
    void run_header_line(Iterator from_, Iterator to_) {
        if ( state::http_code == _state ) {
            std::string temp(from_, to_);
            sscanf_s(temp.c_str(), "HTTP/1.1 %d", &_http_result);
            _state = _http_result == 200 ? state::read_header : state::error;
            return;
        }
        auto sep = std::find(from_, to_, ':');
        if ( sep == to_ ) {
            if ( to_ - from_ > 2 ) {
                _state = state::error;
            } else {
                _state = state::store_content;
            }
            return;
        }
        std::string name(from_, sep);
        std::string value(sep + 2, to_ - 2);
        _header_values[name] = value;

        if ( name == "Transfer-Encoding" ) {
            if ( value == "chunked" ) {
                _content_mode = content_mode::chunked;
            }
        } else if ( name == "Content-Length" ) {
            _content.limit_length_to(std::stoull(value));
        }
    }

    template<typename Iterator>
    Iterator parse_header_line(Iterator from_, Iterator to_) {
        auto start = from_;

        if ( _state == state::store_content ) {
            copy_data(from_, to_);
            return to_;
        }

        auto last = from_;
        for ( ; from_ != to_; ++from_ ) {
            if ( *last == '\r' ) {
                if ( *from_ == '\n' ) {
                    ++from_;
                    break;
                }
            }

            last = from_;
        }

        if ( from_ != to_ ) {
            run_header_line(start, from_);
        }

        return from_;
    }

    template<typename Iterator>
    void parse_header_data(Iterator from_, Iterator to_) {
        while ( from_ != to_ ) {
            from_ = parse_header_line(from_, to_);
        }
    }
public:
    http_state() : _state(state::http_code), _content_mode(content_mode::copy), _http_result(0) {}
    template<typename U>
    http_state(U v_) : _content(std::forward<U>( v_ )), _state(state::http_code), _content_mode(content_mode::copy), _http_result(0) {}

    template<typename Iterator>
    void operator( )( Iterator from_, Iterator to_ ) {
        switch ( _state ) {
        case state::http_code:
        case state::read_header:
            parse_header_data(from_, to_);
            break;
        case state::store_content:
            copy_data(from_, to_);
            break;
        case state::error:
        case state::finished:
            break;
        }
    }

    explicit operator bool () const {
        return _state != state::error;
    }

    const ContentStore& content() const {
        return _content;
    }

    bool is_header_finished() const {
        return _state != state::read_header
            && _state != state::http_code;
    }

    bool is_content_storing() const {
        return _state == state::store_content;
    }

    bool is_finished() const {
        return _state == state::finished;
    }

    const std::unordered_map<std::string, std::string>& header() const {
        return _header_values;
    }

    unsigned http_result() const {
        return _http_result;
    }
};