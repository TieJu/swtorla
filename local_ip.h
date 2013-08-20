#pragma once

#include <vector>
#include <string>
#include <Ws2tcpip.h>

enum class glipa_flags {
    none                = 0,
    include_inactive    = 1 << 0,
    include_loopback    = 1 << 1,
    include_p2p         = 1 << 2,
    require_broadcast   = 1 << 3,
    require_multicast   = 1 << 4
};

inline glipa_flags operator|( glipa_flags lhs_, glipa_flags rhs_ ) {
    return glipa_flags(unsigned( lhs_ ) | unsigned( rhs_ ));
}

inline glipa_flags operator&( glipa_flags lhs_, glipa_flags rhs_ ) {
    return glipa_flags(unsigned( lhs_ ) & unsigned( rhs_ ));
}

inline glipa_flags operator^( glipa_flags lhs_, glipa_flags rhs_ ) {
    return glipa_flags(unsigned( lhs_ ) ^ unsigned( rhs_ ));
}

std::vector<std::wstring> get_local_ip_addresses(glipa_flags flags_ = glipa_flags::none);