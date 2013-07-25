#pragma once

#include "swtor_log_parser.h"
#include "encounter.h"

class combat_analizer {
    std::vector<encounter>  _encounters;


public:
    void add_entry(const combat_log_entry& e_);
};