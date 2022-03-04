#include <set>

#include "Executor.hpp"
#include "ExecutorManager.hpp"
#include "PCodeExecutor.hpp"
#include "BFSExplorationTechnique.hpp"
#include "DFSExplorationTechnique.hpp"

namespace naaz::executor
{

template <class ExecutorPolicy, class ExplorationPolicy>
ExecutorManager<ExecutorPolicy, ExplorationPolicy>::ExecutorManager(
    state::StatePtr initial_state)
    : m_exploration(initial_state), m_executor(initial_state->lifter())
{
}

template <class ExecutorPolicy, class ExplorationPolicy>
std::optional<state::StatePtr>
ExecutorManager<ExecutorPolicy, ExplorationPolicy>::explore(
    std::vector<uint64_t> find, std::vector<uint64_t> avoid)
{
    std::set<uint64_t> find_set(find.begin(), find.end());
    std::set<uint64_t> avoid_set(avoid.begin(), avoid.end());

    while (1) {
        std::optional<state::StatePtr> s = m_exploration.get_next();
        if (!s.has_value())
            break;

        ExecutorResult next_states = m_executor.execute_basic_block(s.value());

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

template <class ExecutorPolicy, class ExplorationPolicy>
std::optional<state::StatePtr>
ExecutorManager<ExecutorPolicy, ExplorationPolicy>::explore(uint64_t find_addr)
{
    std::vector<uint64_t> find_addrs;
    std::vector<uint64_t> avoid_addrs;

    find_addrs.push_back(find_addr);
    return explore(find_addrs, avoid_addrs);
}

template class ExecutorManager<PCodeExecutor, BFSExplorationTechnique>;
template class ExecutorManager<PCodeExecutor, DFSExplorationTechnique>;

} // namespace naaz::executor
