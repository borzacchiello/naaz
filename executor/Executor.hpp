#pragma once

#include <vector>

#include "../state/State.hpp"

namespace naaz::executor
{

struct ExecutorResult {
    std::vector<state::StatePtr> active;
    std::vector<state::StatePtr> exited;
};

} // namespace naaz::executor
