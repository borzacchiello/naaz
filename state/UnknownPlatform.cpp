#include "UnknownPlatform.hpp"
#include "../util/ioutil.hpp"

namespace naaz::state
{

const models::SyscallModel& UnknownPlatform::syscall(int num) const
{
    err("UnknownPlatform") << "syscall(): no syscalls in UnknownPlatform"
                           << std::endl;
    exit_fail();
}

} // namespace naaz::state
