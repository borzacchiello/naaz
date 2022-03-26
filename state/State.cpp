#include "State.hpp"

#include <algorithm>
#include <fstream>

#include "../models/Linker.hpp"

namespace naaz::state
{

State::State(std::shared_ptr<loader::AddressSpace> as,
             std::shared_ptr<lifter::PCodeLifter> lifter, uint64_t pc)
    : m_as(as), m_lifter(lifter), m_pc(pc)
{
    m_linked_functions = std::make_shared<models::LinkedFunctions>();
    m_regs             = std::unique_ptr<MapMemory>(new MapMemory("ram"));
    m_ram = std::unique_ptr<MapMemory>(new MapMemory("ram", as.get()));
    m_fs  = std::unique_ptr<FileSystem>(new FileSystem());

    m_ram->set_solver(&m_solver);

    // Initialize the state (e.g., stack pointer, linked functions)
    arch().init_state(*this);
    models::Linker::The().link(*this);
}

State::State(const State& other)
    : m_as(other.m_as), m_lifter(other.m_lifter),
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

} // namespace naaz::state
