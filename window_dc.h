#pragma once

#include <utility>

#include <Windows.h>

#include <boost/noncopyable.hpp>

class window_dc : boost::noncopyable {
    HWND        _window_handle;
    HDC         _device_handle;

public:
    window_dc()
        : window_dc(nullptr,nullptr) {
    }

    window_dc(HWND window_handle,HDC device_handle) 
        : _window_handle(window_handle)
        , _device_handle(device_handle) {
    }

    window_dc(window_dc&& other)
        : window_dc(nullptr,nullptr) {
        *this = std::move(other);
    }

    window_dc& operator=(window_dc&& other) {
        auto wnd = other.window();
        auto dc = other.release();
        reset(wnd,dc);
        return *this;
    }

    explicit window_dc(HWND window_handle) 
        : window_dc(nullptr,nullptr) {
        reset(window_handle);
    }

    ~window_dc() {
        reset();
    }

    void reset() {
        if ( _window_handle ) {
            if ( _device_handle ) {
                ::ReleaseDC(_window_handle,_device_handle);
                _device_handle = nullptr;
            }
            _window_handle = nullptr;
        }
    }

    void reset(HWND window_handle) {
        reset(window_handle,::GetDC(window_handle));
    }

    void reset(HWND window_handle,HDC device_handle) {
        reset();
        _window_handle = window_handle;
        _device_handle = device_handle;
    }

    HDC get() const {
        return _device_handle;
    }

    explicit operator bool() const {
        return _device_handle != nullptr;
    }

    HWND window() const {
        return _window_handle;
    }

    HDC release() {
        auto copy = _device_handle;
        _device_handle  =nullptr;
        _window_handle = nullptr;
        return copy;
    }
};