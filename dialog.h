#pragma once

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

template<typename Derived>
class dialog_t
: public common_window_base {
private:
    typedef dialog_t<Derived> base;
    static INT_PTR CALLBACK callback_router(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        dialog_t<Derived>* self;
        if ( uMsg == WM_INITDIALOG ) {
            SetWindowLongPtrW(hwndDlg, DWLP_USER, lParam);
            self = reinterpret_cast<dialog_t<Derived>*>( lParam );
        } else {
            self = reinterpret_cast<dialog_t<Derived>*>( GetWindowLongPtrW(hwndDlg, DWLP_USER) );
        }
        if ( self ) {
            return static_cast<Derived*>( self )->on_window_event(uMsg, wParam, lParam);
        } else if ( uMsg == WM_INITDIALOG ) {
            return TRUE;
        }
        return FALSE;
    }

public:
    dialog_t(HINSTANCE hInstance_, LPWSTR lpTemplateName_, HWND hWndParent_) {
        adopt(CreateDialogParamW(hInstance_, lpTemplateName_, hWndParent_, &callback_router, ( LPARAM )this));
        if ( empty() ) {
            auto error = GetLastError();
            throw std::runtime_error("failed got code " + std::to_string(error));
        }
    }
    dialog_t(HINSTANCE hInstance_, LPCDLGTEMPLATE lpTemplate_, HWND hWndParent_) {
        adopt(CreateDialogIndirectParamW(hInstance_, lpTemplate_, hWndParent_, &callback_router, ( LPARAM )this));
        if ( empty() ) {
            auto error = GetLastError();
        }
    }
};

class dialog
    : public common_window_base {
public:
    typedef std::function<LRESULT(dialog* wnd, UINT uMsg, WPARAM wParam, LPARAM lParam)>   callback_type;

private:
    static INT_PTR CALLBACK callback_router(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        dialog* self;
        if ( uMsg == WM_INITDIALOG ) {
            SetWindowLongPtrW(hwndDlg, DWLP_USER, lParam);
            self = reinterpret_cast<dialog* > ( lParam );
        } else {
            self = reinterpret_cast<dialog*>( GetWindowLongPtrW(hwndDlg, DWLP_USER) );
        }
        if ( self && self->_message_handler ) {
            return self->_message_handler(self, uMsg, wParam, lParam);
        } else if ( uMsg == WM_INITDIALOG ) {
            return TRUE;
        }
        return FALSE;
    }

    callback_type   _message_handler;

public:
    dialog(HINSTANCE hInstance_, LPWSTR lpTemplateName_, HWND hWndParent_) {
        adopt(CreateDialogParamW(hInstance_, lpTemplateName_, hWndParent_, &callback_router, ( LPARAM )this));
        if ( empty() ) {
            auto error = GetLastError();
            throw std::runtime_error("failed got code " + std::to_string(error));
        }
    }
    dialog(HINSTANCE hInstance_, LPCDLGTEMPLATE lpTemplate_, HWND hWndParent_) {
        adopt(CreateDialogIndirectParamW(hInstance_, lpTemplate_, hWndParent_, &callback_router, ( LPARAM )this));
        if ( empty() ) {
            auto error = GetLastError();
        }
    }

    template<typename Callback>
    void callback(Callback clb) {
        _message_handler = std::forward<Callback>( clb );
    }

    callback_type callback() {
        return _message_handler;
    }

};