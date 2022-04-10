#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <argparse/argparse.hpp>

#include "../util/config.hpp"
#include "../util/strutil.hpp"
#include "../loader/BFDLoader.hpp"
#include "../expr/ExprBuilder.hpp"
#include "../executor/ExecutorManager.hpp"
#include "../executor/CovExplorationTechnique.hpp"

using namespace naaz;

struct parsed_args_t {
    std::string binpath;
    std::string outdir;

    std::vector<std::string> program_args;
};

static parsed_args_t parse_args_or_die(int argc, char const* argv[])
{
    parsed_args_t res;

    argparse::ArgumentParser program("naaz_path_generator");
    program.add_argument("-o", "--output")
        .default_value<std::string>("/tmp/output")
        .help("Output directory");
    program.add_argument("-P", "--printable_stdin")
        .default_value(false)
        .implicit_value(true)
        .nargs(0)
        .help("Constraint stdin to be printable-only");
    program.add_argument("-T", "--z3_timeout")
        .scan<'i', uint32_t>()
        .help("Set Z3 timeout (ms)");
    program.add_argument("program").help("Path to binary to analyze");
    program.add_argument("args").remaining().help("Program arguments");

    try {
        program.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        exit(1);
    }

    g_config.printable_stdin = program.get<bool>("--printable_stdin");
    if (auto z3_to = program.present<uint32_t>("--z3_timeout"))
        g_config.z3_timeout = *z3_to;

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
    auto args = parse_args_or_die(argc, argv);

    loader::BFDLoader loader(args.binpath);
    state::StatePtr   entry_state = loader.entry_state();
    entry_state->set_argv(args.program_args);

    executor::CovExecutorManager em(entry_state);

    static std::string outdir = args.outdir;
    em.gen_paths([](state::StatePtr s) {
        static uint32_t n_testcase = 0;

        std::string o = string_format("%s/%06u", outdir.c_str(), n_testcase++);
        if (!std::filesystem::is_directory(o) || !std::filesystem::exists(o)) {
            std::filesystem::create_directory(o);
        }
        s->dump_fs(o);
    });
    return 0;
}
