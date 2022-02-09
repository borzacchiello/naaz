#include <cstdlib>
#include "ioutil.hpp"

std::ostream& err(const char* module)
{
    if (module == nullptr)
        module = "nomodule";
    return std::cerr << "!ERR  [" << module << "] ";
}

std::ostream& info(const char* module)
{
    if (module == nullptr)
        module = "nomodule";
    return std::cerr << "!INFO [" << module << "] ";
}

void exit_fail() { exit(-1); }
