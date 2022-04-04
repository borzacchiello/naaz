#pragma once

#include <set>
#include <vector>

#include "ExplorationTechnique.hpp"
#include "ExecutorManager.hpp"

namespace naaz::executor
{

class CovExplorationTechnique final : public ExplorationTechnique
{
    std::set<uint64_t> m_visited_addrs;
    std::set<uint64_t> m_visited_contexts;

    std::vector<state::StatePtr> m_new_addr_queue;
    std::vector<state::StatePtr> m_new_context_queue;
    std::vector<state::StatePtr> m_other_queue;

  public:
    CovExplorationTechnique(state::StatePtr initial_state);

    virtual void add_actives(std::vector<state::StatePtr> states);
    virtual std::optional<state::StatePtr> get_next();

    virtual size_t num_states() const;
};

typedef ExecutorManager<CovExplorationTechnique> CovExecutorManager;

} // namespace naaz::executor
