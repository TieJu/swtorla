#pragma once

#include <string>

#include <Windows.h>

#include <boost/noncopyable.hpp>

class window_class : boost::noncopyable {
public:
    typedef ::ATOM  native_handle_type;

protected:
    std::wstring        _name;
    WNDCLASSEXW         _desc;
    native_handle_type  _class_id;

public:
    window_class(const std::wstring& class_name = L"wnd_class"
                ,HINSTANCE instance = ::GetModuleHandleW(nullptr)
                ,WNDPROC clb = &::DefWindowProc
                ,UINT uflags = 0)
        : _class_id(0) {
        _desc.cbSize = sizeof(_desc);
        callback_proc(clb);
        name(class_name);
        style(uflags);
        class_extra_size(0);
        window_extra_size(0);
        source_instance(instance);
        icon(nullptr);
        mouse_cursor(nullptr);
        smal_icon(nullptr);
        background_brush(nullptr);
        menu_name(nullptr);
    }

    ~window_class() {
        this->unregister_class();
    }

    bool register_class() {
        return ( _class_id = RegisterClassExW(&_desc) ) != 0;
    }

    bool unregister_class() {
        if ( _class_id ) {
            bool ok = TRUE == UnregisterClassW((LPCWSTR)_class_id, _desc.hInstance);
            this->_class_id = 0;
            return ok;
        }
        return true;
    }

    std::wstring name() const { return _name; }
    void name(const std::wstring &name) { _name = name; _desc.lpszClassName = _name.c_str(); }

    void style(UINT flags) { _desc.style=flags; }
    UINT style() const { return _desc.style; }

    void class_extra_size(int iExtraBytes) { _desc.cbClsExtra = iExtraBytes; }
    int class_extra_size() const { return _desc.cbClsExtra; }

    void window_extra_size(int iExtraBytes) { _desc.cbWndExtra = iExtraBytes; }
    int window_extra_size() const { return _desc.cbWndExtra; }

    void source_instance(HINSTANCE hInst) { _desc.hInstance = hInst; }
    HINSTANCE source_instance() const { return _desc.hInstance; }

    void icon(HICON hIc) { _desc.hIcon=hIc; }
    HICON icon() const { return _desc.hIcon; }

    void smal_icon(HICON hIc) { _desc.hIconSm = hIc; }
    HICON smal_icon() const { return _desc.hIconSm; }

    void mouse_cursor(HCURSOR hCr) { _desc.hCursor = hCr; }
    HCURSOR mouse_cursor() const { return _desc.hCursor ;}

    void background_brush(HBRUSH hbr) { _desc.hbrBackground = hbr; }
    HBRUSH background_brush() const { return _desc.hbrBackground; }

    void menu_name(LPCWSTR name) { _desc.lpszMenuName = name; }
    LPCWSTR menu_name() const { return _desc.lpszMenuName; }

    void callback_proc(WNDPROC clb) { _desc.lpfnWndProc = clb; }
    WNDPROC callback_proc() const { return _desc.lpfnWndProc; }

    native_handle_type native_handle() const { return _class_id; }
};