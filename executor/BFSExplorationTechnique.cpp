#include "BFSExplorationTechnique.hpp"

namespace naaz::executor
{

void BFSExplorationTechnique::add_actives(std::vector<state::StatePtr> states)
{
    for (auto s : states)
        m_active.push_front(s);
}

std::optional<state::StatePtr> BFSExplorationTechnique::get_next()
{
    if (m_active.empty())
        return {};

    auto s = m_active.back();
    m_active.pop_back();
    return s;
}

} // namespace naaz::executor
