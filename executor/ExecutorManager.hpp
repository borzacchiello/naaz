#pragma once

#include <optional>
#include <vector>

#include "../state/State.hpp"

namespace naaz::executor
{

template <class ExecutorPolicy, class ExplorationPolicy> class ExecutorManager
{
    ExecutorPolicy    m_executor;
    ExplorationPolicy m_exploration;

  public:
    ExecutorManager(state::StatePtr initial_state);

    std::optional<state::StatePtr> explore(std::vector<uint64_t> find,
                                           std::vector<uint64_t> avoid);
};

} // namespace naaz::executor
