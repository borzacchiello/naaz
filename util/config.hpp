#pragma once

#include <cstdint>

namespace naaz
{

struct Config {
    bool lazy_solving = true;

    uint16_t default_max_n_eval_sym_read  = 256;
    uint16_t default_max_n_eval_sym_write = 64;
};

extern Config g_config;

} // namespace naaz
