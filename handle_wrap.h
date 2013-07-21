#pragma once

#include <functional>

template<typename HandleType,HandleType empty_value = 0>
class handle_wrap {
public:
    typedef HandleType                          value_type;
    typedef value_type*                         pointer_type;
    typedef value_type&                         ref_type;
    typedef std::function<void ( value_type )>  deleter_function;

private:
    value_type          _handle;
    deleter_function    _deleter;

    handle_wrap(const handle_wrap&);
    handle_wrap& operator=(const handle_wrap&);

public:
    handle_wrap() : _handle(empty_value) {}
    handle_wrap(value_type handle_) : _handle(handle_) {}
    handle_wrap(value_type handle_, deleter_function deleter_) : _handle(handle_), _deleter(deleter_) {}
    handle_wrap(handle_wrap&& o_)
        : _handle_wrap() {
        *this = std::move(o_);
    }
    handle_wrap& operator=( handle_wrap&& o_ ) {
        reset(std::move(o_._handle), std::move(o_._deleter));
        o_.release();
        return *this;
    }

    void reset() {
        if ( !empty() ) {
            if ( _deleter ) {
                _deleter(_handle);
                _deleter = deleter_function{};
            }
            _handle = empty_value;
        }
    }

    template<typename U>
    void reset(U v_) {
        reset();
        _handle = std::forward<U>(v_);
    }

    template<typename U,typename V>
    void reset(U w_, V x_) {
        reset();
        _handle = std::forward<U>( w_ );
        _deleter = std::forward<V>( x_ );
    }

    const pointer_type ptr() const {
        return &_handle;
    }

    const pointer_type operator->( ) const {
        return &_handle;
    }

    pointer_type ptr() {
        return &_handle;
    }

    pointer_type operator->( ) {
        return &_handle;
    }

    ref_type get() {
        return _handle;
    }

    const ref_type get() const {
        return _handle;
    }

    ref_type operator*( ) {
        return _handle;
    }

    value_type release() {
        auto copy = _handle;
        _handle = empty_value;
        _deleter = deleter_function{};
        return copy;
    }

    const ref_type operator*( ) const {
        return _handle;
    }

    bool empty() const {
        return _handle == empty_value;
    }
};