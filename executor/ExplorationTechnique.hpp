#pragma once

#include <optional>
#include <vector>
#include "../state/State.hpp"

namespace naaz::executor
{

class ExplorationTechnique
{
  protected:
    state::StatePtr              m_initial_state;
    std::vector<state::StatePtr> m_exited_states;
    std::vector<state::StatePtr> m_avoided_states;

  public:
    ExplorationTechnique(state::StatePtr state) : m_initial_state(state) {}

    virtual std::optional<state::StatePtr> get_next() = 0;

    virtual void add_actives(std::vector<state::StatePtr> states) = 0;
    void         add_exited(state::StatePtr s);
    void         add_avoided(state::StatePtr s);

    const std::vector<state::StatePtr>& exited() const
    {
        return m_exited_states;
    }

    const std::vector<state::StatePtr>& avoided() const
    {
        return m_avoided_states;
    }

    virtual size_t num_states() const
    {
        return m_exited_states.size() + m_avoided_states.size();
    }
};

} // namespace naaz::executor
