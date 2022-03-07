#include "DFSExplorationTechnique.hpp"

namespace naaz::executor
{

void DFSExplorationTechnique::add_actives(std::vector<state::StatePtr> states)
{
    for (auto s : states)
        m_active.push_back(s);
}

std::optional<state::StatePtr> DFSExplorationTechnique::get_next()
{
    if (m_active.empty())
        return {};

    auto s = m_active.back();
    m_active.pop_back();
    return s;
}

template class ExecutorManager<DFSExplorationTechnique>;

} // namespace naaz::executor
