#include <cstdio>
#include <cstdint>
#include <filesystem>

#include "../loader/BFDLoader.hpp"
#include "../expr/ExprBuilder.hpp"
#include "../executor/ExecutorManager.hpp"
#include "../executor/RandDFSExplorationTechnique.hpp"

using namespace naaz;

static void usage(const char* name)
{
    fprintf(stderr, "%s <find> <binary>\n", name);
    exit(1);
}

int main(int argc, char const* argv[])
{
    if (argc < 3)
        usage(argv[0]);

    uint64_t    find_addr = std::strtoul(argv[1], NULL, 16);
    const char* binpath   = argv[2];

    std::vector<std::string> state_argv;
    for (int i = 2; i < argc; ++i) {
        state_argv.push_back(argv[i]);
    }

    loader::BFDLoader loader(binpath);
    state::StatePtr   entry_state = loader.entry_state();
    entry_state->set_argv(state_argv);

    executor::RandDFSExecutorManager em(entry_state);

    std::optional<state::StatePtr> s = em.explore(find_addr);
    if (s.has_value()) {
        std::string out_dir = "/tmp/output";
        fprintf(stdout, "state found! dumping proof to %s\n", out_dir.c_str());
        if (!std::filesystem::is_directory(out_dir) ||
            !std::filesystem::exists(out_dir)) {
            std::filesystem::create_directory(out_dir);
        }
        s.value()->dump_fs(out_dir);
    } else
        fprintf(stdout, "state not found\n");

    fprintf(stdout, "generated states: %lu\n", em.num_states());
    return 0;
}
