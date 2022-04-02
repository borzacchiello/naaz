#include "strutil.hpp"

#include <sstream>

namespace naaz
{

std::vector<std::string> split_at(const std::string& str, char c)
{
    std::stringstream        test(str);
    std::string              segment;
    std::vector<std::string> seglist;

    while (std::getline(test, segment, c)) {
        seglist.push_back(segment);
    }
    return seglist;
}

} // namespace naaz
