#pragma once

#include <windows.h>

class os_info {
public:
    enum class windows_type {
        earlier_thant_xp,
        xp,
        vista,
        _2k8,
        _7,
        _2k8r2,
        _8,
        _2k12,
        _8_1,
        _2k12r2,
        later_than_8_1
    };

    windows_type    _type;
    bool            _is_64bit;

    void find_windows_type();
    void check_64_bit_os();


public:
    os_info()
        : _type(windows_type::earlier_thant_xp)
        , _is_64bit(false) {
        find_windows_type();
        check_64_bit_os();
    }

    windows_type type() const {
        return _type;
    }

    bool is_64bit() const {
        return _is_64bit;
    }
};