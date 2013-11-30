#pragma once

#include <vector>
#include <string>
#include <WinSock2.h>
#include <Windows.h>

enum class glipa_flags {
    /**
     * Empty flag
     */
    none                = 0,
    /**
     * Include inactive nis.
     */
    include_inactive    = 1 << 0,
    /**
     * Include loopback nis.
     */
    include_loopback    = 1 << 1,
    /**
     * Include peer to peer connections.
     */
    include_p2p         = 1 << 2,
    /**
     * Only nis with broadcast capability.
     */
    require_broadcast   = 1 << 3,
    /**
     * Only nis with multicast capability.
     */
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

/**
 * Gets the ip addresses of all net adapters.
 * Assumes winsok is allready initialized.
 * @param[in] flags_    Flags to filter devices.
 * @return              Vector of ip addresses (v4 and v6 are posible).
 */
std::vector<std::wstring> get_local_ip_addresses(glipa_flags flags_ = glipa_flags::none);