#pragma once

#include "window_dc.h"


#include <string>
#include <tuple>
#include <functional>

#include <Windows.h>

#include <boost/noncopyable.hpp>

class common_window_base
: boost::noncopyable {
public:
    typedef HWND        natvie_handle_value_type;

private:
    natvie_handle_value_type    _handle;

protected:
    common_window_base(natvie_handle_value_type handle_) : _handle(handle_) {}
    common_window_base() : common_window_base(nullptr) {}
    ~common_window_base() {
        adopt(nullptr);
    }

    void adopt(natvie_handle_value_type handle_) {
        if ( _handle ) {
            ::DestroyWindow(_handle);
        }
        _handle = handle_;
    }

public:
    bool empty() const {
        return _handle == nullptr;
    }
    
    void size(int x_, int y_) {
        ::SetWindowPos(_handle, NULL, 0, 0, x_, y_, SWP_NOMOVE | SWP_NOZORDER);

    }

    std::tuple<int, int> size() {
        RECT r;
        ::GetWindowRect(_handle, &r);

        return std::make_tuple(int( r.right - r.left ), int( r.bottom - r.top ));
    }

    void pos(int x_, int y_) {
        ::SetWindowPos(_handle, NULL, x_, y_, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }

    void move(int x_, int y_, int width_, int height_, bool repaint_ = true) {
        ::MoveWindow(_handle, x_, y_, width_, height_, repaint_);
    }

    std::tuple<int, int> pos() {
        RECT r;
        ::GetWindowRect(_handle, &r);
        return std::make_tuple(int( r.left ), int( r.top ));
    }

    RECT client_area_rect() {
        RECT r;
        ::GetClientRect(_handle, &r);
        return r;
    }

    void caption(const std::wstring& name_) {
        ::SetWindowTextW(_handle, name_.c_str());
    }

    std::wstring caption() {
        std::wstring buf;
        buf.resize(::GetWindowTextLengthW(_handle) + 1, L' ');
        ::GetWindowTextW(_handle, const_cast<wchar_t*>(buf.data()), int( buf.size() ));
        return buf;
    }

    void maximize() {
        ShowWindow(_handle, SW_MAXIMIZE);
    }
    void minimize() {
        ShowWindow(_handle, SW_MINIMIZE);
    }
    void normal() {
        ShowWindow(_handle, SW_SHOW);
    }
    void hide() {
        ShowWindow(_handle, SW_HIDE);
    }

    void enable(bool enable_ = true) {
        ::EnableWindow(_handle, enable_);
    }

    void disable() {
        enable(false);
    }

    bool is_enabled() {
        return TRUE == ::IsWindowEnabled(_handle);
    }

    void style(DWORD style_) {
        ::SetWindowLongPtrW(_handle, GWL_STYLE, (LONG_PTR)style_);
        ::SetWindowPos(_handle, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }

    DWORD style() {
        return ( DWORD )::GetWindowLongPtrW(_handle, GWL_STYLE);
    }

    void ext_style(DWORD style_) {
        ::SetWindowLongPtrW(_handle, GWL_EXSTYLE, (LONG_PTR)style_);
        ::SetWindowPos(_handle, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }

    DWORD ext_style() {
        return ( DWORD )::GetWindowLongPtrW(_handle, GWL_EXSTYLE);
    }

    HWND native_handle() const {
        return _handle;
    }

    HWND native_window_handle() const {
        return _handle;
    }

    window_dc dc() {
        return window_dc(_handle);
    }

    void update() {
        ::UpdateWindow(_handle);
    }

    HINSTANCE instance_handle() {
        return reinterpret_cast<HINSTANCE>( GetWindowLongPtrW(_handle, GWLP_HINSTANCE) );
    }
};