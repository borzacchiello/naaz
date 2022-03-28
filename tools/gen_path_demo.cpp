#include <cstdio>
#include <cstdint>
#include <filesystem>

#include "../util/strutil.hpp"
#include "../loader/BFDLoader.hpp"
#include "../expr/ExprBuilder.hpp"
#include "../executor/ExecutorManager.hpp"
#include "../executor/RandDFSExplorationTechnique.hpp"

using namespace naaz;

static void usage(const char* name)
{
    fprintf(stderr, "%s <binary> [<arg> ...]\n", name);
    exit(1);
}

int main(int argc, char const* argv[])
{
    if (argc < 2)
        usage(argv[0]);

    const char* binpath = argv[1];

    std::vector<std::string> state_argv;
    for (int i = 1; i < argc; ++i) {
        state_argv.push_back(argv[i]);
    }

    loader::BFDLoader loader(binpath);
    state::StatePtr   entry_state = loader.entry_state();
    entry_state->set_argv(state_argv);

    executor::RandDFSExecutorManager em(entry_state);

    static std::string out_dir = "/tmp/output";
    if (!std::filesystem::is_directory(out_dir) ||
        !std::filesystem::exists(out_dir)) {
        std::filesystem::create_directory(out_dir);
    }

    em.gen_paths([](state::StatePtr s) {
        static uint32_t n_testcase = 0;

        std::string o = string_format("%s/%06u", out_dir.c_str(), n_testcase++);
        if (!std::filesystem::is_directory(o) || !std::filesystem::exists(o)) {
            std::filesystem::create_directory(o);
        }
        s->dump_fs(o);
    });
    return 0;
}
