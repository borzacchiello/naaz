#pragma once

#include <string>

#include "../executor/Executor.hpp"

namespace naaz::models
{

class SyscallModel
{
  protected:
    std::string m_name;

    SyscallModel(const std::string& name) : m_name(name) {}

  public:
    const std::string& name() const { return m_name; }

    virtual void exec(state::StatePtr           s,
                      executor::ExecutorResult& o_successors) const = 0;
};

} // namespace naaz::models
