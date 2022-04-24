#include "Arch.hpp"

#include "../expr/ExprBuilder.hpp"

namespace naaz
{

std::filesystem::path Arch::getSleighProcessorDir()
{
    char* env_val = std::getenv("SLEIGH_PROCESSORS");
    if (env_val)
        return std::filesystem::path(env_val);

    // Change this
    return std::filesystem::path(
        "/home/luca/git/naaz/third_party/sleigh/processors");
}

void Arch::set_return(state::StatePtr s, uint64_t addr) const
{
    set_return(s, expr::ExprBuilder::The().mk_const(addr, ptr_size()));
}

} // namespace naaz
