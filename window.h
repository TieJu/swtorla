#pragma once

#include "window_class.h"
#include "common_window_base.h"
#include "window_dc.h"

#include <utility>
#include <vector>
#include <tuple>
#include <string>
#include <functional>
#include <Windows.h>
#include <unordered_map>

#include <inplace_any.h>

#include <boost/noncopyable.hpp>

inline std::string GetWindowTextA( HWND hwnd_ ) {
    std::string text;
    text.resize( ::GetWindowTextLengthA( hwnd_ ), ' ' );
    ::GetWindowTextA( hwnd_, const_cast<char*>( text.data() ), int( text.size() ) + 1 );
    return text;
}

inline std::wstring GetWindowTextW( HWND hwnd_ ) {
    std::wstring text;
    text.resize( ::GetWindowTextLengthW( hwnd_ ), L' ' );
    ::GetWindowTextW( hwnd_, const_cast<wchar_t*>( text.data() ), int( text.size() ) + 1 );
    return text;
}

class window : public common_window_base {
public:
    typedef std::function<LRESULT(window* wnd, UINT uMsg, WPARAM wParam, LPARAM lParam)>   callback_type;

private:
    enum {
        max_window_data = 16,
    };
    callback_type                                                       _message_handler;
    std::unordered_map<std::wstring,tj::inplace_any<max_window_data>>   _user_data;

    static LRESULT CALLBACK callback_router(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        auto _this = reinterpret_cast<window*>( ::GetWindowLongPtrW(hwnd, GWLP_USERDATA) );
        if ( !_this || !_this->_message_handler ) {
            return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
        } else {
            return _this->_message_handler(_this, uMsg, wParam, lParam);
        }
    }

public:
    window(DWORD ext_style
          ,const std::wstring &class_name
          ,const std::wstring &caption
          ,DWORD style
          ,int x
          ,int y
          ,int width
          ,int height
          ,HWND parent
          ,HMENU menu
          ,HINSTANCE instance) {
        adopt(::CreateWindowExW(ext_style, class_name.c_str(), caption.c_str(), style, x, y, width, height, parent, menu, instance, nullptr));

        if ( empty() ) {
            throw std::runtime_error("unable to create window");
        }

        ::SetWindowLongPtrW(native_handle(), GWLP_USERDATA, ( LONG_PTR )this);
        //::SetWindowLongPtrW(native_handle(), GWLP_WNDPROC, (LONG_PTR)callback_router);
    }
    window(DWORD ext_style
          ,::ATOM class_name
          ,const std::wstring &caption
          ,DWORD style
          ,int x
          ,int y
          ,int width
          ,int height
          ,HWND parent
          ,HMENU menu
          ,HINSTANCE instance) {
        adopt(::CreateWindowExW(ext_style, (LPCWSTR)class_name, caption.c_str(), style, x, y, width, height, parent, menu, instance, nullptr));

        if ( empty() ) {
            throw std::runtime_error("unable to create window");
        }

        ::SetWindowLongPtrW(native_handle(), GWLP_USERDATA, ( LONG_PTR )this);
        //::SetWindowLongPtrW(native_handle(), GWLP_WNDPROC, (LONG_PTR)callback_router);
    }

    window(DWORD ext_style
          ,const window_class& class_name
          ,const std::wstring &caption
          ,DWORD style
          ,int x
          ,int y
          ,int width
          ,int height
          ,HWND parent
          ,HMENU menu
          ,HINSTANCE instance)
          : window(ext_style, class_name.native_handle(), caption, style, x, y, width, height, parent, menu, instance) {}

    /*~window() {
    }*/

    template<typename Callback>
    void callback(Callback clb) {
        _message_handler = std::forward<Callback>( clb );
        ::SetWindowLongPtrW(native_handle(), GWLP_WNDPROC, (LONG_PTR)callback_router);
    }

    callback_type callback() {
        return _message_handler;
    }

    template<typename U>
    void insert_user_data(const std::wstring& name_,U v_) {
        _user_data[name_] = std::forward<U>( v_ );
    }

    template<typename U>
    U* get_user_data(const std::wstring& name_) {
        auto at = _user_data.find(name_);
        if ( at == end(_user_data) ) {
            return nullptr;
        }
        return tj::any_cast<U>( &at->second );
    }

    void erase_user_data(const std::wstring& name_) {
        _user_data.erase(name_);
    }
};