#include "parseutil.hpp"

#include <cstdlib>

namespace naaz
{

bool parse_uint(const char* arg, uint64_t* out)
{
    const char* needle = arg;
    while (*needle == ' ' || *needle == '\t')
        needle++;

    char*    res;
    uint64_t num = strtoul(needle, &res, 0);
    if (needle == res)
        return false; // no character

    *out = num;
    return true;
}

} // namespace naaz
