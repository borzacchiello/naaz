#include "Arch.hpp"
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

namespace arch
{

std::filesystem::path x86_64::getSleighSLA() const
{
    return getSleighProcessorDir() /
           std::filesystem::path("x86/data/languages/x86-64.sla");
}

std::filesystem::path x86_64::getSleighPSPEC() const
{
    return getSleighProcessorDir() /
           std::filesystem::path("x86/data/languages/x86-64.pspec");
}

} // namespace arch
} // namespace naaz
