#pragma once

#include <string>

#include <Windows.h>

struct program_version {
    unsigned    major;
    unsigned    minor;
    unsigned    patch;
    unsigned    build;
};

program_version find_version_info(LPCWSTR resource_name_);