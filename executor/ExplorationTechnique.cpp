#include "ExplorationTechnique.hpp"

namespace naaz::executor
{

void ExplorationTechnique::add_exited(state::StatePtr s)
{
    m_exited_states.push_back(s);
}

void ExplorationTechnique::add_avoided(state::StatePtr s)
{
    m_avoided_states.push_back(s);
}

} // namespace naaz::executor
