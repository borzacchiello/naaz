#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <argparse/argparse.hpp>

#include "../util/strutil.hpp"
#include "../util/parseutil.hpp"
#include "../loader/BFDLoader.hpp"
#include "../expr/ExprBuilder.hpp"
#include "../executor/ExecutorManager.hpp"
#include "../executor/RandDFSExplorationTechnique.hpp"

using namespace naaz;

struct parsed_args_t {
    std::string binpath;
    std::string outdir;

    std::vector<uint64_t> find_addrs;
    std::vector<uint64_t> avoid_addrs;

    std::vector<std::string> program_args;
};

static parsed_args_t parse_args_or_die(int argc, char const* argv[])
{
    parsed_args_t res;

    argparse::ArgumentParser program("naaz_finder");

    program.add_argument("-f", "--find")
        .required()
        .help("Addresses to reach (comma-separated, hex format)");
    program.add_argument("-a", "--avoid")
        .help("Address to avoid (comma-separated, hex format)");
    program.add_argument("-o", "--output")
        .default_value<std::string>("/tmp/output")
        .help("Output directory");
    program.add_argument("program").help("Path to binary to analyze");
    program.add_argument("args").remaining().help("Program arguments");

    try {
        program.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        exit(1);
    }

    res.outdir = program.get("--output");
    if (!std::filesystem::is_directory(res.outdir) ||
        !std::filesystem::exists(res.outdir)) {
        if (res.outdir == "/tmp/output")
            std::filesystem::create_directory(res.outdir);
        else {
            fprintf(stderr, "the output directory %s does not exist",
                    res.outdir.c_str());
            exit(1);
        }
    }

    res.binpath = program.get("program");

    auto find_addrs_str = program.get("--find");
    for (auto a : split_at(find_addrs_str, ',')) {
        uint64_t addr;
        if (!parse_uint(a.c_str(), &addr)) {
            fprintf(stderr, "Invalid find addr\n");
            exit(2);
        }
        res.find_addrs.push_back(addr);
    }

    if (auto avoid_addrs_str = program.present("--avoid")) {
        for (auto a : split_at(*avoid_addrs_str, ',')) {
            uint64_t addr;
            if (!parse_uint(a.c_str(), &addr)) {
                fprintf(stderr, "Invalid avoid addr\n");
                exit(3);
            }
            res.avoid_addrs.push_back(addr);
        }
    }

    res.program_args.push_back(program.get("program"));
    if (program.present("args")) {
        for (auto& arg : program.get<std::vector<std::string>>("args")) {
            res.program_args.push_back(arg);
        }
    }

    return res;
}

int main(int argc, char const* argv[])
{
    auto res = parse_args_or_die(argc, argv);

    loader::BFDLoader loader(res.binpath);
    state::StatePtr   entry_state = loader.entry_state();
    entry_state->set_argv(res.program_args);

    executor::RandDFSExecutorManager em(entry_state);

    std::optional<state::StatePtr> s =
        em.explore(res.find_addrs, res.avoid_addrs);
    if (s.has_value()) {
        fprintf(stdout, "state found! dumping proof to %s\n",
                res.outdir.c_str());
        s.value()->dump_fs(res.outdir);
    } else
        fprintf(stdout, "state not found\n");

    fprintf(stdout, "generated states: %lu\n", em.num_states());
    return 0;
}
