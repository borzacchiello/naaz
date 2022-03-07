#pragma once

#include <list>

#include "ExplorationTechnique.hpp"
#include "ExecutorManager.hpp"

namespace naaz::executor
{

class BFSExplorationTechnique final : public ExplorationTechnique
{
    std::list<state::StatePtr> m_active;

  public:
    BFSExplorationTechnique(state::StatePtr initial_state)
        : ExplorationTechnique(initial_state)
    {
        m_active.push_back(initial_state);
    }

    virtual void add_actives(std::vector<state::StatePtr> states);
    virtual std::optional<state::StatePtr> get_next();
};

typedef ExecutorManager<BFSExplorationTechnique> BFSExecutorManager;

} // namespace naaz::executor
