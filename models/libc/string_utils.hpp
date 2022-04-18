#pragma once

#include "../../expr/Expr.hpp"

namespace naaz::models::libc
{

struct resolved_string_t {
    state::StatePtr state;
    expr::BVExprPtr str;

    resolved_string_t(state::StatePtr state_, expr::BVExprPtr str_)
        : state(state_), str(str_)
    {
    }
};

std::vector<resolved_string_t> resolve_string(state::StatePtr state,
                                              uint64_t        str_addr,
                                              int             max_forks = 32,
                                              int             max_size  = -1);

} // namespace naaz::models::libc
