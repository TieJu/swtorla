#pragma once

#include <Windows.h>
#include <utility>

class socket_address {
    struct sockaddr _data;
    int             _size { sizeof( sockaddr ) };

public:
    socket_address() { _data.sa_family = AF_UNSPEC; std::fill( std::begin( _data.sa_data ), std::end( _data.sa_data ), 0 ); }
    socket_address( const socket_address& ) = default;
    socket_address& operator=( const socket_address& ) = default;
    sockaddr* data() { return &_data; }
    const sockaddr* data() const { return &_data; }
    int size() const { return _size; }
    int* size_ptr() { return &_size; }
};

class socket_address_inet
    : public socket_address {
public:
    socket_address_inet() {
        data()->sa_family = AF_INET;
        *size_ptr() = sizeof( sockaddr_in );
        ip() = INADDR_ANY;
    }

    u_short port() const { return reinterpret_cast<const sockaddr_in*>( data() )->sin_port; }
    u_short& port() { return reinterpret_cast<sockaddr_in*>( data() )->sin_port; }

    u_long ip() const { return reinterpret_cast<const sockaddr_in*>( data() )->sin_addr.S_un.S_addr; }
    u_long& ip() { return reinterpret_cast<sockaddr_in*>( data() )->sin_addr.S_un.S_addr; }

    bool is_any() const { return ip() == INADDR_ANY; }
    bool is_none() const { return ip() == INADDR_NONE; }
};