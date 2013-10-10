#pragma once


#include <string>
#include <vector>
#include <locale>

// not super std conform, inject some to_string functions...
namespace std {
template<typename Type>
inline std::string to_string(const Type& value) {
    std::ostringstream buf;

    buf << value << std::flush;

    return buf.str();
}

inline const std::string &to_string(const std::string& value) {
    return value;
}

inline std::string to_string(const char* value) {
    return value;
}

inline std::string to_string(bool b) {
    return b ? "true" : "false";
}

inline std::string to_string(const std::wstring& value) {
    const std::locale locale;
    typedef std::codecvt<wchar_t, char, std::mbstate_t> converter_type;
    const auto &converter = std::use_facet<converter_type>(locale);
    std::vector<char> to(value.length() * converter.max_length());
    std::mbstate_t state;
    const wchar_t* from_next;
    char* to_next;
    const auto result = converter.out(state
                                     ,value.data()
                                     ,value.data() + value.length()
                                     ,from_next
                                     ,to.data()
                                     ,to.data() + to.size()
                                     ,to_next);
    if (result == converter_type::ok || result == converter_type::noconv) {
        return std::string(to.data(),to_next);
    }
    return std::string();
}

inline std::string to_string(const wchar_t* value) {
    const auto len = wcslen(value);
    const std::locale locale;
    typedef std::codecvt<wchar_t, char, std::mbstate_t> converter_type;
    const auto &converter = std::use_facet<converter_type>(locale);
    std::vector<char> to(len * converter.max_length());
    std::mbstate_t state;
    const wchar_t* from_next;
    char* to_next;
    const auto result = converter.out(state
                                     ,value
                                     ,value + len
                                     ,from_next
                                     ,to.data()
                                     ,to.data() + to.size()
                                     ,to_next);
    if (result == converter_type::ok || result == converter_type::noconv) {
        return std::string(to.data(),to_next);
    }
    return std::string();
}

template<typename Type>
inline std::wstring to_wstring( const Type& value ) {
    std::owstringstream buf;

    buf << value << std::flush;

    return buf.str();
}

inline const std::wstring &to_wstring( const std::wstring& value ) {
    return value;
}

inline std::wstring to_wstring( const wchar_t* value ) {
    return value;
}

inline std::wstring to_wstring( bool b ) {
    return b ? L"true" : L"false";
}

inline std::wstring to_wstring( const std::string& value ) {
    const std::locale locale;
    typedef std::codecvt<char, wchar_t, std::mbstate_t> converter_type;
    const auto &converter = std::use_facet<converter_type>( locale );
    std::vector<wchar_t> to( value.length() * converter.max_length() );
    std::mbstate_t state;
    const char* from_next;
    wchar_t* to_next;
    const auto result = converter.out( state
                                       , value.data()
                                       , value.data() + value.length()
                                       , from_next
                                       , to.data()
                                       , to.data() + to.size()
                                       , to_next );
    if ( result == converter_type::ok || result == converter_type::noconv ) {
        return std::wstring( to.data(), to_next );
    }
    return std::wstring();
}

inline std::wstring to_wstring( const char* value ) {
    const auto len = strlen( value );
    const std::locale locale;
    typedef std::codecvt<char, wchar_t, std::mbstate_t> converter_type;
    const auto &converter = std::use_facet<converter_type>( locale );
    std::vector<wchar_t> to( len * converter.max_length() );
    std::mbstate_t state;
    const char* from_next;
    wchar_t* to_next;
    const auto result = converter.out( state
                                       , value
                                       , value + len
                                       , from_next
                                       , to.data()
                                       , to.data() + to.size()
                                       , to_next );
    if ( result == converter_type::ok || result == converter_type::noconv ) {
        return std::wstring( to.data(), to_next );
    }
    return std::wstring();
}
}