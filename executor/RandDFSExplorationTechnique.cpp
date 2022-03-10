#include "RandDFSExplorationTechnique.hpp"

#include <cstdlib>

namespace naaz::executor
{

static void shuffle(std::vector<state::StatePtr>& vec)
{
    static bool initialized = false;
    if (!initialized) {
        std::srand(0x42424242);
        initialized = true;
    }

    if (vec.size() <= 1)
        return;

    for (size_t i = 0; i < vec.size() - 1; ++i) {
        size_t j   = i + std::rand() / (RAND_MAX / (vec.size() - i) + 1);
        auto   tmp = vec[j];
        vec[j]     = vec[i];
        vec[i]     = tmp;
    }
}

void RandDFSExplorationTechnique::add_actives(
    std::vector<state::StatePtr> states)
{
    shuffle(states);
    for (auto s : states)
        m_active.push_back(s);
}

std::optional<state::StatePtr> RandDFSExplorationTechnique::get_next()
{
    if (m_active.empty())
        return {};

    auto s = m_active.back();
    m_active.pop_back();
    return s;
}

template class ExecutorManager<RandDFSExplorationTechnique>;

} // namespace naaz::executor
