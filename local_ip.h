#pragma once

#include <vector>
#include <string>
#include <Ws2tcpip.h>

// include_mask_: ored with INTERFACE_INFO::iiFlags if the result is zero the interface info will be ignored
// exclude_mask_: ored with INTERFACE_INFO::iiFlags if the result is nonzero the interface info will be ignored
// default is: only active interfaces, and no loopback and point2point interfaces
std::vector<std::wstring> get_local_ip_addresses(u_long include_mask_ = IFF_UP, u_long exclude_mask_ = IFF_LOOPBACK | IFF_POINTTOPOINT);