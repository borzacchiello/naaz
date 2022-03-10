#pragma once

#include <vector>

#include "ExplorationTechnique.hpp"
#include "ExecutorManager.hpp"

namespace naaz::executor
{

class RandDFSExplorationTechnique final : public ExplorationTechnique
{
    std::vector<state::StatePtr> m_active;

  public:
    RandDFSExplorationTechnique(state::StatePtr initial_state)
        : ExplorationTechnique(initial_state)
    {
        m_active.push_back(initial_state);
    }

    virtual void add_actives(std::vector<state::StatePtr> states);
    virtual std::optional<state::StatePtr> get_next();

    virtual size_t num_states() const
    {
        return m_active.size() + ExplorationTechnique::num_states();
    }
};

typedef ExecutorManager<RandDFSExplorationTechnique> RandDFSExecutorManager;

} // namespace naaz::executor
