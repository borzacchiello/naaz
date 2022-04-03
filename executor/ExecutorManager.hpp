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
                if (find_set.contains(s->pc())) {
                    if (s->satisfiable() == solver::CheckResult::SAT)
                        return s;
                } else if (!avoid_set.contains(s->pc()))
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

    void gen_paths(void (*callback)(state::StatePtr))
    {
        // Generate states, and call the `callback` when a state exits
        while (1) {
            std::optional<state::StatePtr> s = m_exploration.get_next();
            if (!s.has_value())
                break;

            ExecutorResult next_states =
                m_executor.execute_basic_block(s.value());

            for (auto s : next_states.exited)
                if (s->satisfiable() == solver::CheckResult::SAT)
                    callback(s);

            m_exploration.add_actives(next_states.active);
            // std::cout << "num states: " << num_states() << std::endl;
        }
    }

    size_t num_states() const { return m_exploration.num_states(); }
};

} // namespace naaz::executor
