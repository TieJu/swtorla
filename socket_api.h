#pragma once
#include <Windows.h>

#include <memory>
#include <string>
#include <exception>

#include <boost/scope_exit.hpp>

#include "socket_address.h"

class socket_api {
private:
    struct state {
        WSADATA data;

        state() {
            if ( WSAStartup( MAKEWORD( 2, 2 ), &data ) ) {
                int error = 1;
            }
        }

        ~state() {
            WSACleanup();
        }
    };

    static std::weak_ptr<state>     __state;
    std::shared_ptr<state>          _state;

public:
    socket_api()
        : _state( __state.lock() ) {
        if ( !_state ) {
            _state = std::make_shared<state>();
            __state = _state;
        }
    }
    ~socket_api() = default;
    /*
    socket_address get_host_by_name( const std::string& name_ ) {
        socket_address_inet adr;
        adr.port() = 0;
        adr.ip() = ::inet_addr( name_.c_str() );
        if ( adr.ip() == INADDR_NONE ) {
            auto* list = gethostbyname( name_.c_str() );
            adr.ip() = *reinterpret_cast<unsigned long*>( list->h_addr_list[0] );
        }
        return adr;
    }*/

    std::string get_error( ) { return get_error_meessage( WSAGetLastError( ) ); }
    void check_error() {
        auto error = WSAGetLastError();
        if ( was_error( error ) ) {
            throw std::runtime_error( "winsock error: " + get_error_meessage( error ) );
        }
    }

    socket_address_inet inet_address( const std::string& ip_ ) {
        socket_address_inet addr;

        addr.ip() = inet_addr( ip_.c_str() );

        return addr;
    }

    bool was_error( HRESULT error_code_ ) {
        if ( error_code_ >= 0 ) {
            return false;
        }
        if ( EWOULDBLOCK == error_code_ ) {
            return false;
        }

        return true;
    }
    std::string get_error_meessage( HRESULT error_code_ ) {
        LPSTR text = nullptr;
        FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, error_code_, 0, (LPSTR)&text, 0, nullptr );

        std::string result { text };
        LocalFree( text );
        return result;
    }
};