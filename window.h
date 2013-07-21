#pragma once

#include "window_class.h"
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

class window : boost::noncopyable {
public:
    typedef std::function<LRESULT(window* wnd, UINT uMsg, WPARAM wParam, LPARAM lParam)>   callback_type;

private:
    enum {
        max_window_data = 16,
    };
    HWND                                                                _window_handle;
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
                _window_handle = ::CreateWindowExW(ext_style, class_name.c_str(), caption.c_str(), style, x, y, width, height, parent, menu, instance, nullptr);

                if ( !_window_handle ) {
                    throw std::runtime_error("unable to create window");
                }

                ::SetWindowLongPtrW(_window_handle, GWLP_USERDATA, ( LONG_PTR )this);
                //::SetWindowLongPtrW(_window_handle, GWLP_WNDPROC, (LONG_PTR)callback_router);
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
            _window_handle = ::CreateWindowExW(ext_style, (LPCWSTR)class_name, caption.c_str(), style, x, y, width, height, parent, menu, instance, nullptr);

            if ( !_window_handle ) {
                throw std::runtime_error("unable to create window");
            }

            ::SetWindowLongPtrW(_window_handle, GWLP_USERDATA, ( LONG_PTR )this);
            //::SetWindowLongPtrW(_window_handle, GWLP_WNDPROC, (LONG_PTR)callback_router);
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

    ~window() {
        if ( _window_handle ) {
            ::DestroyWindow(_window_handle);
        }
    }

    template<typename Callback>
    void callback(Callback clb) {
        _message_handler = std::forward<Callback>( clb );
        ::SetWindowLongPtrW(_window_handle, GWLP_WNDPROC, (LONG_PTR)callback_router);
    }

    callback_type callback() {
        return _message_handler;
    }

    void size(int x, int y) {
        ::SetWindowPos(_window_handle, NULL, 0, 0, x, y, SWP_NOMOVE | SWP_NOZORDER);

    }

    std::tuple<int, int> size() {
        RECT r;
        ::GetWindowRect(_window_handle, &r);

        return std::make_tuple(int( r.right - r.left ), int( r.bottom - r.top ));
    }

    void pos(int x, int y) {
        ::SetWindowPos(_window_handle, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }

    void move(int x_, int y_, int width_, int height_, bool repaint_ = true) {
        ::MoveWindow(_window_handle, x_, y_, width_, height_, repaint_);
    }

    std::tuple<int, int> pos() {
        RECT r;
        ::GetWindowRect(_window_handle, &r);
        return std::make_tuple(int( r.left ), int( r.top ));
    }

    RECT get_client_area_rect() {
        RECT r;
        ::GetClientRect(_window_handle, &r);
        return r;
    }

    void caption(const std::wstring& name) {
        ::SetWindowTextW(_window_handle, name.c_str());
    }

    std::wstring caption() {
        std::vector<wchar_t> buf;
        buf.resize(::GetWindowTextLengthW(_window_handle) + 1);
        ::GetWindowTextW(_window_handle, buf.data(), int( buf.size() ));
        return std::wstring(buf.begin(), buf.end());
    }

    void maximize() {
        ShowWindow(_window_handle, SW_MAXIMIZE);
    }
    void minimize() {
        ShowWindow(_window_handle, SW_MINIMIZE);
    }
    void normal() {
        ShowWindow(_window_handle, SW_SHOW);
    }
    void hide() {
        ShowWindow(_window_handle, SW_HIDE);
    }
    //void Fullscreen();

    void style(DWORD style) {
        ::SetWindowLongPtrW(_window_handle, GWL_STYLE, (LONG_PTR)style);
        ::SetWindowPos(_window_handle, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }

    DWORD style() {
        return ( DWORD )::GetWindowLongPtrW(_window_handle, GWL_STYLE);
    }

    void ext_style(DWORD style) {
        ::SetWindowLongPtrW(_window_handle, GWL_EXSTYLE, (LONG_PTR)style);
        ::SetWindowPos(_window_handle, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }

    DWORD ext_style() {
        return ( DWORD )::GetWindowLongPtrW(_window_handle, GWL_EXSTYLE);
    }

    HWND native_window_handle() {
        return _window_handle;
    }

    window_dc dc() {
        return window_dc(_window_handle);
    }

    void update() {
        ::UpdateWindow(_window_handle);
    }

    HINSTANCE instance_handle() {
        return reinterpret_cast<HINSTANCE>( GetWindowLongPtrW(_window_handle, GWLP_HINSTANCE) );
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