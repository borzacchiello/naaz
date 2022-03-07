#pragma once

#include "../Linker.hpp"

namespace naaz::models::libc
{

// FIXME: verbose... Find a way to generate this code

class libc_start_main final : public Model
{
    libc_start_main() : Model("__libc_start_main", CallConv::CDECL) {}

  public:
    static const libc_start_main& The()
    {
        static const libc_start_main v;
        return v;
    }

    virtual void exec(state::StatePtr           s,
                      executor::ExecutorResult& o_successors) const;
};

class exit final : public Model
{
    exit() : Model("exit", CallConv::CDECL) {}

  public:
    static const exit& The()
    {
        static const exit v;
        return v;
    }

    virtual void exec(state::StatePtr           s,
                      executor::ExecutorResult& o_successors) const;
};

} // namespace naaz::models::libc

#define REGISTER_LIBC_FUNCTIONS                                                \
    l.register_model(libc::libc_start_main::The().name(),                      \
                     &libc::libc_start_main::The());                           \
    l.register_model(libc::exit::The().name(), &libc::exit::The());
