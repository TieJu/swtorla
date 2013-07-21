#pragma once

#include "window.h"

#include <CommCtrl.h>

class progress_bar
    : public window {

public:
    progress_bar(int x_, int y_, int width_, int height_, HWND parent_, unsigned id_, HINSTANCE instance_)
        : window(0, PROGRESS_CLASS, L"", WS_CHILD | WS_VISIBLE, x_, y_, width_, height_, parent_, (HMENU)id_, instance_) {

    }

    void range(unsigned from_, unsigned to_) {
        ::PostMessageW(native_window_handle(), PBM_SETRANGE32, from_, to_);
    }
    std::tuple<unsigned, unsigned> range() const {
        PBRANGE r{};
        ::SendMessageW(native_window_handle(), PBM_GETRANGE, 0, (LPARAM)&r);
        return std::make_tuple(r.iLow, r.iHigh);
    }

    void progress(unsigned pos_) {
        ::PostMessageW(native_window_handle(), PBM_SETPOS, pos_, 0);
    }
    unsigned progress() const {
        return (unsigned)::SendMessageW(native_window_handle(), PBM_GETPOS, 0, 0);
    }

    void marque(bool enable_, bool display_ = true, unsigned interval_ = 0) {
        if ( enable_ ) {
            style(style() | PBS_MARQUEE);
            ::PostMessageW(native_window_handle(), PBM_SETMARQUEE, display_, interval_);
        } else {
            style(style() & ~PBS_MARQUEE);
        }
    }

    enum class display_state {
        normal,
        error,
        paused,
    };
    void state(display_state state_) {
#ifndef PBM_SETSTATE 
#define PBM_SETSTATE            (WM_USER+16)
#define PBM_GETSTATE            (WM_USER+17)
#define PBST_NORMAL             0x0001
#define PBST_ERROR              0x0002
#define PBST_PAUSED             0x0003
#endif
        WPARAM value = 0;
        switch ( state_ ) {
        case display_state::normal:
            value = PBST_NORMAL;
            break;
        case display_state::error:
            value = PBST_ERROR;
            break;
        case display_state::paused:
            value = PBST_PAUSED;
            break;
        }
        ::PostMessageW(native_window_handle(), PBM_SETSTATE, value, 0);
    }
    display_state state() const {
        switch ( ::SendMessageW(native_window_handle(), PBM_GETSTATE, 0, 0) ) {
        default:
        case PBST_NORMAL:
            return display_state::normal;
        case PBST_ERROR:
            return display_state::error;
        case PBST_PAUSED:
            return display_state::paused;
        }
    }
};

struct update_progress_error_event {
    progress_bar::display_state   level;
};