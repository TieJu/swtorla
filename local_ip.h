#pragma once

#include <vector>
#include <string>

std::vector<std::wstring> get_local_ip_addresses(bool include_inactive_ = false,bool include_loopback_ = false, bool include_pointtopoint_ = false);