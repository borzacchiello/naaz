#pragma once

#include <optional>
#include <vector>

#include "PCodeExecutor.hpp"
#include "../state/State.hpp"

namespace naaz::executor
{

template <class ExplorationPolicy> class ExecutorManager
{
    PCodeExecutor     m_executor;
    ExplorationPolicy m_exploration;

  public:
    ExecutorManager(state::StatePtr initial_state)
        : m_exploration(initial_state), m_executor(initial_state->lifter())
    {
    }

    std::optional<state::StatePtr> explore(std::vector<uint64_t> find,
                                           std::vector<uint64_t> avoid)
    {
        std::set<uint64_t> find_set(find.begin(), find.end());
        std::set<uint64_t> avoid_set(avoid.begin(), avoid.end());

        while (1) {
            std::optional<state::StatePtr> s = m_exploration.get_next();
            if (!s.has_value())
                break;

            ExecutorResult next_states =
                m_executor.execute_basic_block(s.value());

            for (auto s : next_states.exited)
                m_exploration.add_exited(s);

            std::vector<state::StatePtr> active;
            for (auto s : next_states.active) {
                if (find_set.contains(s->pc()))
                    return s;
                if (!avoid_set.contains(s->pc()))
                    active.push_back(s);
            }

            m_exploration.add_actives(active);
        }

        return {};
    }

    std::optional<state::StatePtr> explore(uint64_t find_addr)
    {
        std::vector<uint64_t> find_addrs;
        std::vector<uint64_t> avoid_addrs;

        find_addrs.push_back(find_addr);
        return explore(find_addrs, avoid_addrs);
    }
};

} // namespace naaz::executor
