#include "local_ip.h"
#include <WinSock2.h>

#include <boost/scope_exit.hpp>

std::vector<std::wstring> get_local_ip_addresses(u_long include_mask_ /*= IFF_UP*/, u_long exclude_mask_ /*= IFF_LOOPBACK | IFF_POINTTOPOINT*/) {
    std::vector<std::wstring> result;

    auto sock = WSASocketW(AF_INET, SOCK_DGRAM, 0, 0, 0, 0);
    if ( sock == SOCKET_ERROR ) {
        return result;
    }

    BOOST_SCOPE_EXIT_ALL(= ) {
        ::closesocket(sock);
    };

    std::vector<INTERFACE_INFO> buffer(1);
    DWORD bytes_written = 0;
    for ( ;; ) {
        if ( ::WSAIoctl(sock, SIO_GET_INTERFACE_LIST, nullptr, 0, buffer.data(), buffer.size() * sizeof(INTERFACE_INFO), &bytes_written, nullptr, nullptr ) == SOCKET_ERROR) {
            auto error = WSAGetLastError();
            if ( error == WSAEFAULT ) {
                buffer.resize(buffer.size() * 2);
            } else {
                return result;
            }
        } else {
            break;
        }
    }

    buffer.resize(bytes_written / sizeof( INTERFACE_INFO ));

    std::vector<wchar_t> textbuffer;

    for ( auto& interface_info : buffer ) {
        if ( !( interface_info.iiFlags & include_mask_ ) ) {
            continue;
        }
        if ( interface_info.iiFlags & exclude_mask_ ) {
            continue;
        }
        DWORD length = 0;
        WSAAddressToStringW(reinterpret_cast<SOCKADDR*>( &interface_info.iiAddress ), sizeof( interface_info.iiAddress ), nullptr, nullptr, &length);
        textbuffer.resize(length);
        WSAAddressToStringW(reinterpret_cast<SOCKADDR*>( &interface_info.iiAddress ), sizeof( interface_info.iiAddress ), nullptr, textbuffer.data(), &length);
        result.emplace_back(textbuffer.data());
    }

    return result;
}