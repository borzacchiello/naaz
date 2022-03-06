#pragma once

#include "../Linker.hpp"

namespace naaz::models::libc
{

// FIXME: verbose... Find a way to generate this code

class libc_start_main final : public Model
{
    libc_start_main(const std::string& name, CallConv cv) : Model(name, cv) {}

  public:
    static const libc_start_main& The();

    virtual bool returns() const;
    virtual void exec(state::State& s) const;
};

} // namespace naaz::models::libc

#define REGISTER_LIBC_FUNCTIONS                                                \
    l.register_model(libc::libc_start_main::The().name(),                      \
                     &libc::libc_start_main::The());
