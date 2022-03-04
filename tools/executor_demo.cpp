#include <cstdio>
#include <cstdint>

#include "../loader/BFDLoader.hpp"
#include "../executor/ExecutorManager.hpp"
#include "../executor/DFSExplorationTechnique.hpp"
#include "../executor/PCodeExecutor.hpp"

using namespace naaz;

static void usage(const char* name)
{
    fprintf(stderr, "%s <binary> <find>\n", name);
    exit(1);
}

int main(int argc, char const* argv[])
{
    if (argc < 3)
        usage(argv[0]);

    const char* binpath   = argv[1];
    uint64_t    find_addr = std::strtoul(argv[2], NULL, 16);

    loader::BFDLoader loader(binpath);
    state::StatePtr   entry_state = loader.entry_state();

    executor::ExecutorManager<executor::PCodeExecutor,
                              executor::DFSExplorationTechnique>
        em(entry_state);

    std::optional<state::StatePtr> s = em.explore(find_addr);

    fprintf(stdout, "found state? %d\n", s.has_value());
    return 0;
}
