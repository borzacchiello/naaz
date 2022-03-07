#pragma once

#include <vector>

#include "ExplorationTechnique.hpp"
#include "ExecutorManager.hpp"

namespace naaz::executor
{

class DFSExplorationTechnique final : public ExplorationTechnique
{
    std::vector<state::StatePtr> m_active;

  public:
    DFSExplorationTechnique(state::StatePtr initial_state)
        : ExplorationTechnique(initial_state)
    {
        m_active.push_back(initial_state);
    }

    virtual void add_actives(std::vector<state::StatePtr> states);
    virtual std::optional<state::StatePtr> get_next();
};

typedef ExecutorManager<DFSExplorationTechnique> DFSExecutorManager;

} // namespace naaz::executor
