#pragma once

#include <string>
#include <map>

#include "../executor/Executor.hpp"

namespace naaz
{

namespace state
{
class State;
}

enum CallConv { CDECL };

namespace models
{

class Model
{
  protected:
    std::string m_name;
    CallConv    m_call_conv;

    Model(const std::string& name, CallConv call_conv)
        : m_name(name), m_call_conv(call_conv)
    {
    }

  public:
    const std::string& name() const { return m_name; }

    virtual void exec(state::StatePtr           s,
                      executor::ExecutorResult& o_successors) const = 0;
};

struct LinkedFunctions {
    std::map<uint64_t, const Model*> links;
};

} // namespace models

} // namespace naaz
