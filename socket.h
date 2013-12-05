#pragma once

#include <Windows.h>
#include <utility>

#include "socket_api.h"
#include "socket_address.h"

class c_socket {
    SOCKET      _socket = INVALID_SOCKET;
    socket_api  _api;
    HWND        _event_rcv = nullptr;

public:
    c_socket() = default;
    c_socket( const socket_api& api_, SOCKET socket_ ) : _api(api_), _socket( socket_ ) {}
    c_socket( const socket_api& api_, int af_, int type_, int proto_ ) : _api( api_ ), _socket( ::socket( af_, type_, proto_ ) ) {}

    c_socket( const c_socket& ) = delete;
    c_socket& operator=( const c_socket& ) = delete;

    c_socket( c_socket&& other_ ) : c_socket() { *this = std::move( other_ ); }
    c_socket& operator=( c_socket&& other_ ) {
        using std::swap;
        swap( _api, other_._api );
        swap( _event_rcv, other_._event_rcv );
        swap( _socket, other_._socket );
        return *this;
    }

    ~c_socket() { reset(); }

    SOCKET release() {
        SOCKET s = INVALID_SOCKET;
        std::swap( s, _socket );
        _event_rcv = nullptr;
        return s;
    }

    void reset( const socket_api& api_, SOCKET socket_ ) {
        reset();
        _api = api_;
        _socket = socket_;
    }

    void reset() {
        if ( _socket != INVALID_SOCKET ) {
            if ( _event_rcv ) {
                WSAAsyncSelect( _socket, _event_rcv, 0, 0 );
            }
            closesocket( _socket );
            _socket = INVALID_SOCKET;
        }
    }

    explicit operator bool()const { return INVALID_SOCKET != _socket; }

    SOCKET get() const { return _socket; }

    void swap( c_socket& other_ ) {
        using std::swap;
        swap( _api, other_._api );
        swap( _event_rcv, other_._event_rcv );
        swap( _socket, other_._socket );
    }

    template<typename Type>
    bool send( const Type& value_, int flags_ = 0 ) {
        return sizeof( value_ ) == send( &value_, sizeof( value_ ), flags_ );
    }

    int send( const void* buf_, int len_, int flags_ = 0 ) {
        return ::send( _socket, reinterpret_cast<const char*>(buf_), len_, flags_ );
    }

    int recive( void* buf_, int len_, int flags_ = 0 ) {
        return ::recv( _socket, reinterpret_cast<char*>( buf_ ), len_, flags_ );
    }

    template<typename InetAddrType>
    bool bind( InetAddrType addr_ ) {
        return 0 == ::bind( _socket, addr_.data(), addr_.size() );
    }
    template<typename InetAddrType>
    bool connect( InetAddrType addr_ ) {
        return 0 == ::connect( _socket, addr_.data(), addr_.size() );
    }

    socket_address name() const {
        socket_address adr;
        ::getsockname( _socket, adr.data(), adr.size_ptr() );
        return adr;
    }
    socket_address peer() const {
        socket_address adr;
        ::getpeername( _socket, adr.data(), adr.size_ptr() );
        return adr;
    }

    template<typename InetAddrType>
    int send_to( const void* buf_, int len_, int flags_, InetAddrType addr_ ) {
        return ::sendto( _socket, reinterpret_cast<const char*>(buf_), len_, flags_, adr_.data(), adr_.size_ptr() );
    }
    std::pair<int, socket_address> recive_from( void* buf_, int len_, int flags_ ) {
        socket_address adr;
        return std::make_pair( ::recvfrom( _socket, reinterpret_cast<char*>(buf_), len_, flags_, adr.data(), adr.size_ptr() ), adr );
    }

    bool listen( int backlog_ = SOMAXCONN ) { return 0 == ::listen( _socket, backlog_ ); }

    c_socket accept() { return c_socket { _api, ::accept( _socket, nullptr, nullptr ) }; }

    bool set_opt( int level_, int name_, const char* value_, int len_ ) { return 0 == setsockopt( _socket, level_, name_, value_, len_ ); }
    bool get_opt( int level_, int name_, char* value_, int& len_ ) { return 0 == getsockopt( _socket, level_, name_, value_, &len_ ); }
    template<typename Value>
    bool set_opt( int level_, int name_, Value value_ ) { return set_opt( level_, name_, reinterpret_cast<const char*>( &value_ ), sizeof( value_ ) ); }
    template<typename Value>
    bool get_opt( int level_, int name_, Value& value_ ) { return get_opt( level_, name_, reinterpret_cast<char*>( &value_ ), sizeof( value_ ) ); }
    bool io_ctrl( long cmd_, u_long* args_ ) { return 0 == ioctlsocket( _socket, cmd_, args_ ); }
    enum class modes {
        // not complete list, add this on demand
        so_keep_allive,
        tcp_no_delay,
        file_io_blocking_io,
    };
    bool set_mode( modes mode_, bool boolean_ ) {
        switch ( mode_ ) {
        case modes::so_keep_allive:
            return set_opt( SOL_SOCKET, SO_KEEPALIVE, boolean_ ? 1L : 0L );
        case modes::tcp_no_delay:
            return set_opt( IPPROTO_TCP, TCP_NODELAY, boolean_ ? 1L : 0L );
        case modes::file_io_blocking_io:
            u_long value = boolean_ ? 0UL : 1UL;
            return io_ctrl( FIONBIO, &value );
        }
        return false;
    }

    bool async_select( HWND hwnd_, unsigned int msg_, long event_mask_ ) {
        if ( _event_rcv ) {
            WSAAsyncSelect( _socket, _event_rcv, 0, 0 );
        }
        _event_rcv = hwnd_;
        return 0 == WSAAsyncSelect( _socket, hwnd_, msg_, event_mask_ );
    }

    void shutdown( int mode_ ) {
        ::shutdown( _socket, mode_ );
    }
};

inline void swap( c_socket& l_, c_socket& r_ ) {
    c_socket t = std::move( l_ );
    l_ = std::move( r_ );
    r_ = std::move( t );
}