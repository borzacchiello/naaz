#pragma once

#include <exception>
#include <vector>
#include <memory>

namespace naaz::state
{
class State;
typedef std::shared_ptr<State> StatePtr;
} // namespace naaz::state

namespace naaz::executor
{

struct ExecutorResult {
    std::vector<state::StatePtr> active;
    std::vector<state::StatePtr> exited;
};

struct UnsatStateException : public std::exception {
    const char* what() const throw() { return "UNSAT State"; }
};

} // namespace naaz::executor
