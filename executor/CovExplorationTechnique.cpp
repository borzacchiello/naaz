#include "CovExplorationTechnique.hpp"
#include "ExecutorManager.hpp"

#include "../third_party/xxHash/xxh3.h"

namespace naaz::executor
{

static uint64_t get_context_checksum(state::StatePtr state)
{
    XXH64_state_t chk_state;
    XXH64_reset(&chk_state, 0);

    auto pc = state->pc();
    XXH64_update(&chk_state, &pc, sizeof(pc));
    for (auto addr : state->stacktrace())
        XXH64_update(&chk_state, &addr, sizeof(addr));
    return XXH64_digest(&chk_state);
}

CovExplorationTechnique::CovExplorationTechnique(state::StatePtr initial_state)
    : ExplorationTechnique(initial_state)
{
    m_new_addr_queue.push_back(initial_state);
    m_visited_addrs.insert(initial_state->pc());
    m_visited_contexts.insert(get_context_checksum(initial_state));
}

void CovExplorationTechnique::add_actives(std::vector<state::StatePtr> states)
{
    for (auto s : states) {
        auto context_chk = get_context_checksum(s);
        if (!m_visited_addrs.contains(s->pc()))
            m_new_addr_queue.push_back(s);
        else if (!m_visited_contexts.contains(context_chk))
            m_new_context_queue.push_back(s);
        else
            m_other_queue.push_back(s);

        m_visited_addrs.insert(s->pc());
        m_visited_contexts.insert(context_chk);
    }
}

std::optional<state::StatePtr> CovExplorationTechnique::get_next()
{
    if (!m_new_addr_queue.empty()) {
        auto s = m_new_addr_queue.back();
        m_new_addr_queue.pop_back();
        return s;
    }

    if (!m_new_context_queue.empty()) {
        auto s = m_new_context_queue.back();
        m_new_context_queue.pop_back();
        return s;
    }

    if (!m_other_queue.empty()) {
        auto s = m_other_queue.back();
        m_other_queue.pop_back();
        return s;
    }

    return {};
}

size_t CovExplorationTechnique::num_states() const
{
    return m_new_addr_queue.size() + m_new_context_queue.size() +
           m_other_queue.size() + ExplorationTechnique::num_states();
}

} // namespace naaz::executor
