#pragma once

#include <set>
#include <memory>
#include <filesystem>

#include "MapMemory.hpp"
#include "FileSystem.hpp"
#include "Solver.hpp"
#include "../loader/AddressSpace.hpp"
#include "../lifter/PCodeLifter.hpp"
#include "../solver/ConstraintManager.hpp"
#include "../expr/Expr.hpp"
#include "../models/Linker.hpp"

namespace naaz::state
{

class State;
typedef std::shared_ptr<State> StatePtr;
class State
{
    uint64_t m_pc;
    uint64_t m_heap_ptr;

    std::vector<uint64_t>        m_stacktrace;
    std::vector<expr::BVExprPtr> m_argv;

    std::unique_ptr<MapMemory>  m_regs;
    std::unique_ptr<MapMemory>  m_ram;
    std::unique_ptr<FileSystem> m_fs;

    std::shared_ptr<loader::AddressSpace>    m_as;
    std::shared_ptr<lifter::PCodeLifter>     m_lifter;
    std::shared_ptr<models::LinkedFunctions> m_linked_functions;

    Solver m_solver;

    uint64_t m_libc_start_main_exit_wrapper = 0;

  public:
    State(std::shared_ptr<loader::AddressSpace> as,
          std::shared_ptr<lifter::PCodeLifter> lifter, uint64_t pc);
    State(const State& other);
    ~State() {}

    const Arch& arch() const { return m_lifter->arch(); }
    std::shared_ptr<lifter::PCodeLifter>  lifter() { return m_lifter; }
    std::shared_ptr<loader::AddressSpace> address_space() { return m_as; }

    bool get_code_at(uint64_t addr, uint8_t** o_data, uint64_t* o_size);

    expr::BVExprPtr read(expr::BVExprPtr addr, size_t len);
    expr::BVExprPtr read(uint64_t addr, size_t len);
    void            write(expr::BVExprPtr addr, expr::BVExprPtr data);
    void            write(uint64_t addr, expr::BVExprPtr data);

    expr::BVExprPtr read_buf(expr::BVExprPtr addr, size_t len);
    expr::BVExprPtr read_buf(uint64_t addr, size_t len);
    void            write_buf(expr::BVExprPtr addr, expr::BVExprPtr data);
    void            write_buf(uint64_t addr, expr::BVExprPtr data);

    expr::BVExprPtr reg_read(const std::string& name);
    expr::BVExprPtr reg_read(uint64_t offset, size_t size);
    void            reg_write(const std::string& name, expr::BVExprPtr data);
    void            reg_write(uint64_t offset, expr::BVExprPtr data);

    expr::BVExprPtr get_int_param(CallConv cv, uint64_t i);

    void register_linked_function(uint64_t addr, const models::Model* m);
    bool is_linked_function(uint64_t addr);
    const models::Model* get_linked_model(uint64_t addr);
    uint64_t             get_libc_start_main_exit_wrapper_address() const
    {
        return m_libc_start_main_exit_wrapper;
    }

    FileSystem& fs() { return *m_fs; }
    bool        dump_fs(std::filesystem::path out_dir);

    Solver&             solver() { return m_solver; }
    expr::BoolExprPtr   pi() const;
    solver::CheckResult satisfiable();

    void     set_pc(uint64_t pc) { m_pc = pc; }
    uint64_t pc() const { return m_pc; }

    void register_call(uint64_t retaddr) { m_stacktrace.push_back(retaddr); }
    void register_ret()
    {
        if (m_stacktrace.empty())
            return;
        m_stacktrace.pop_back();
    }

    StatePtr clone() const { return std::shared_ptr<State>(new State(*this)); }

    uint64_t allocate(uint64_t size);
    uint64_t allocate(expr::ExprPtr size);

    const std::vector<expr::BVExprPtr>& get_argv() const { return m_argv; }
    void set_argv(const std::vector<std::string>& argv);
    void set_argv(const std::vector<expr::BVExprPtr>& argv) { m_argv = argv; }

    // exited stuff
    bool    exited  = false;
    int32_t retcode = 0;
};

} // namespace naaz::state
