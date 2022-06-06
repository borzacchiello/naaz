#include <cstdlib>
#include "ioutil.hpp"

std::ostream& err(const char* module)
{
    if (module == nullptr)
        return std::cerr << "!ERR ";
    return std::cerr << "!ERR  [" << module << "] ";
}

std::ostream& info(const char* module)
{
    if (module == nullptr)
        return std::cerr << "!INFO ";
    return std::cerr << "!INFO [" << module << "] ";
}

std::ostream& dbg(const char* module)
{
    if (module == nullptr)
        return std::cerr << "!DBG ";
    return std::cerr << "!DBG [" << module << "] ";
}

std::ostream& pp_stream() { return std::cout; }

[[noreturn]] void exit_fail() { exit(-1); }
