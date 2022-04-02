#include "State.hpp"

#include <algorithm>
#include <fstream>

#include "../models/Linker.hpp"
#include "../expr/ExprBuilder.hpp"
#include "../util/ioutil.hpp"
#include "../util/strutil.hpp"

namespace naaz::state
{

State::State(std::shared_ptr<loader::AddressSpace> as,
             std::shared_ptr<lifter::PCodeLifter> lifter, uint64_t pc)
    : m_as(as), m_lifter(lifter), m_pc(pc)
{
    m_linked_functions = std::make_shared<models::LinkedFunctions>();
    m_regs             = std::unique_ptr<MapMemory>(new MapMemory("regs"));
    m_ram = std::unique_ptr<MapMemory>(new MapMemory("ram", as.get()));
    m_fs  = std::unique_ptr<FileSystem>(new FileSystem());

    m_ram->set_solver(&m_solver);

    // Initialize the state (e.g., stack pointer, linked functions)
    m_heap_ptr = arch().get_heap_ptr();

    arch().init_state(*this);
    models::Linker::The().link(*this);
}

State::State(const State& other)
    : m_as(other.m_as), m_lifter(other.m_lifter), m_pc(other.m_pc),
      m_heap_ptr(other.m_heap_ptr), m_argv(other.m_argv),
      m_linked_functions(other.m_linked_functions), m_solver(other.m_solver)
{
    m_ram  = other.m_ram->clone();
    m_regs = other.m_regs->clone();
    m_fs   = other.m_fs->clone();
    m_ram->set_solver(&m_solver);
}

bool State::get_code_at(uint64_t addr, uint8_t** o_data, uint64_t* o_size)
{
    // FIXME: This should be changed... It's wrong in too many ways
    return m_as->get_ref(addr, o_data, o_size);
}

expr::BVExprPtr State::read(expr::BVExprPtr addr, size_t len)
{
    return m_ram->read(addr, len, arch().endianess());
}

expr::BVExprPtr State::read(uint64_t addr, size_t len)
{
    return m_ram->read(addr, len, arch().endianess());
}

void State::write(expr::BVExprPtr addr, expr::BVExprPtr data)
{
    m_ram->write(addr, data, arch().endianess());
}

void State::write(uint64_t addr, expr::BVExprPtr data)
{
    m_ram->write(addr, data, arch().endianess());
}

expr::BVExprPtr State::read_buf(expr::BVExprPtr addr, size_t len)
{
    return m_ram->read(addr, len, Endianess::BIG);
}

expr::BVExprPtr State::read_buf(uint64_t addr, size_t len)
{
    return m_ram->read(addr, len, Endianess::BIG);
}

void State::write_buf(expr::BVExprPtr addr, expr::BVExprPtr data)
{
    m_ram->write(addr, data, Endianess::BIG);
}

void State::write_buf(uint64_t addr, expr::BVExprPtr data)
{
    m_ram->write(addr, data, Endianess::BIG);
}

expr::BVExprPtr State::reg_read(const std::string& name)
{
    csleigh_Varnode reg_varnode = m_lifter->reg(name);
    return reg_read(reg_varnode.offset, reg_varnode.size);
}

expr::BVExprPtr State::reg_read(uint64_t offset, size_t size)
{
    return m_regs->read(offset, size, Endianess::LITTLE);
}

void State::reg_write(const std::string& name, expr::BVExprPtr data)
{
    csleigh_Varnode reg_varnode = m_lifter->reg(name);
    return reg_write(reg_varnode.offset, data);
}

void State::reg_write(uint64_t offset, expr::BVExprPtr data)
{
    m_regs->write(offset, data, Endianess::LITTLE);
}

expr::BVExprPtr State::get_int_param(CallConv cv, uint64_t i)
{
    return arch().get_int_param(cv, *this, i);
}

void State::register_linked_function(uint64_t addr, const models::Model* m)
{
    m_linked_functions->links[addr] = m;
    if (m->name() == "libc_start_main_exit_wrapper")
        m_libc_start_main_exit_wrapper = addr;
}

bool State::is_linked_function(uint64_t addr)
{
    return m_linked_functions->links.contains(addr);
}

const models::Model* State::get_linked_model(uint64_t addr)
{
    return m_linked_functions->links[addr];
}

expr::BoolExprPtr State::pi() const { return m_solver.manager().pi(); }

void State::dump_fs(std::filesystem::path out_dir)
{
    // dump stacktrace
    std::filesystem::path out_file = out_dir / "stacktrace.txt";
    auto stacktrace_file           = std::fstream(out_file, std::ios::out);
    for (auto addr : m_stacktrace)
        stacktrace_file << "0x" << std::hex << addr << std::endl;
    stacktrace_file.close();

    // dump exit code
    out_file            = out_dir / "exit_code.txt";
    auto exit_code_file = std::fstream(out_file, std::ios::out);
    exit_code_file << std::dec << retcode << std::endl;
    exit_code_file.close();

    // dump argv
    out_file       = out_dir / "argv.txt";
    auto argv_file = std::fstream(out_file, std::ios::out);
    int  i         = 0;
    for (auto arg : m_argv) {
        argv_file << std::dec << i++ << ": ";
        auto arg_eval = m_solver.evaluate(arg);
        auto arg_data = arg_eval.as_data();
        for (int j = 0; j < arg_eval.size() / 8U; ++j) {
            if ((int)arg_data[j] >= 32 && (int)arg_data[j] <= 126) {
                argv_file << arg_data[j];
            } else {
                argv_file << "\\x" << std::setw(2) << std::setfill('0')
                          << std::hex << (int)arg_data[j];
            }
        }
        argv_file << std::endl;
    }
    argv_file.close();

#if 0
    out_file     = out_dir / "pi.txt";
    auto pi_file = std::fstream(out_file, std::ios::out);
    pi_file << pi()->to_string() << std::endl;
    pi_file.close();
#endif

    for (File* f : m_fs->files()) {
        if (f->size() == 0)
            continue;

        std::string filename(f->name());
        std::replace(filename.begin(), filename.end(), '/', '_');

        out_file  = out_dir / (filename + ".bin");
        auto fout = std::fstream(out_file, std::ios::out | std::ios::binary);

        auto data_expr = f->read_all();
        auto data_eval = m_solver.evaluate(data_expr);
        auto data      = data_eval.as_data();
        fout.write((const char*)data.data(), data.size());
        fout.close();
    }
}

uint64_t State::allocate(uint64_t size)
{
    auto res = m_heap_ptr;
    m_heap_ptr += size;
    return m_heap_ptr;
}

uint64_t State::allocate(expr::ExprPtr size)
{
    if (size->kind() != expr::Expr::Kind::CONST) {
        err("State") << "allocate(): symbolic size is not supported"
                     << std::endl;
        exit_fail();
    }

    auto size_ = std::static_pointer_cast<const expr::ConstExpr>(size);
    return allocate(size_->val().as_u64());
}

static bool parse_uint32(const char* arg, uint64_t* out)
{
    const char* needle = arg;
    while (*needle == ' ' || *needle == '\t')
        needle++;

    char*    res;
    uint64_t num = strtoul(needle, &res, 0);
    if (needle == res)
        return false; // no character
    if (errno != 0)
        return false; // error while parsing

    *out = num;
    return res;
}

void State::set_argv(const std::vector<std::string>& argv)
{
    m_argv.clear();

    static int argv_sym_idx = 0;
    for (const auto& s : argv) {
        if (s.starts_with("@@:")) {
            auto arg_tokens = split_at(s, ':');
            if (arg_tokens.size() > 3) {
                err("State")
                    << "set_argv(): [1] invalid '@@:<arg>:<size>' directive"
                    << std::endl;
                exit_fail();
            }

            bool printable_only = false;
            bool alphan_only    = false;
            bool non_zero       = false;

            std::string size_str;
            if (arg_tokens.size() == 2) {
                size_str = arg_tokens.at(1);
            } else {
                size_str = arg_tokens.at(2);
                if (arg_tokens.at(1) == "print") {
                    printable_only = true;
                } else if (arg_tokens.at(1) == "anum") {
                    alphan_only = true;
                } else if (arg_tokens.at(1) == "nozero") {
                    non_zero = true;
                } else {
                    err("State")
                        << "set_argv(): [2] invalid '@@:<arg>:<size>' directive"
                        << std::endl;
                    exit_fail();
                }
            }

            uint64_t size;
            if (!parse_uint32(size_str.c_str(), &size) || size == 0) {
                err("State")
                    << "set_argv(): [3] invalid '@@:<arg>:<size>' directive"
                    << std::endl;
                exit_fail();
            }

            expr::BVExprPtr arg_expr = nullptr;
            for (uint64_t i = 0; i < size; ++i) {
                auto sym = expr::ExprBuilder::The().mk_sym(
                    string_format("argv_%d[%lu]", argv_sym_idx, i), 8);

                if (printable_only) {
                    m_solver.add(expr::ExprBuilder::The().mk_uge(
                        sym, expr::ExprBuilder::The().mk_const(0x20UL, 8)));
                    m_solver.add(expr::ExprBuilder::The().mk_ule(
                        sym, expr::ExprBuilder::The().mk_const(0x7eUL, 8)));
                } else if (alphan_only) {
                    auto cond1 = expr::ExprBuilder::The().mk_bool_and(
                        expr::ExprBuilder::The().mk_uge(
                            sym, expr::ExprBuilder::The().mk_const(0x30UL, 8)),
                        expr::ExprBuilder::The().mk_ule(
                            sym, expr::ExprBuilder::The().mk_const(0x39UL, 8)));
                    auto cond2 = expr::ExprBuilder::The().mk_bool_and(
                        expr::ExprBuilder::The().mk_uge(
                            sym, expr::ExprBuilder::The().mk_const(0x41UL, 8)),
                        expr::ExprBuilder::The().mk_ule(
                            sym, expr::ExprBuilder::The().mk_const(0x5AUL, 8)));
                    auto cond3 = expr::ExprBuilder::The().mk_bool_and(
                        expr::ExprBuilder::The().mk_uge(
                            sym, expr::ExprBuilder::The().mk_const(0x61UL, 8)),
                        expr::ExprBuilder::The().mk_ule(
                            sym, expr::ExprBuilder::The().mk_const(0x7AUL, 8)));
                    auto constraint =
                        expr::ExprBuilder::The().mk_bool_or(cond1, cond2);
                    constraint =
                        expr::ExprBuilder::The().mk_bool_or(constraint, cond3);
                    m_solver.add(constraint);
                } else if (non_zero) {
                    auto constraint = expr::ExprBuilder::The().mk_not(
                        expr::ExprBuilder::The().mk_eq(
                            sym, expr::ExprBuilder::The().mk_const(0UL, 8)));
                    m_solver.add(constraint);
                }

                if (i == 0)
                    arg_expr = sym;
                else
                    arg_expr =
                        expr::ExprBuilder::The().mk_concat(arg_expr, sym);
            }
            argv_sym_idx++;

            arg_expr = expr::ExprBuilder::The().mk_concat(
                arg_expr, expr::ExprBuilder::The().mk_const(0UL, 8));
            m_argv.push_back(arg_expr);
        } else
            m_argv.push_back(expr::ExprBuilder::The().mk_const(
                expr::BVConst((const uint8_t*)s.data(), s.size() + 1)));
    }
}

} // namespace naaz::state
