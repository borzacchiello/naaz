#pragma once

#include <set>
#include <memory>

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

    std::unique_ptr<MapMemory>  m_regs;
    std::unique_ptr<MapMemory>  m_ram;
    std::unique_ptr<FileSystem> m_fs;

    std::shared_ptr<loader::AddressSpace>    m_as;
    std::shared_ptr<lifter::PCodeLifter>     m_lifter;
    std::shared_ptr<models::LinkedFunctions> m_linked_functions;

    Solver m_solver;

    uint64_t m_exit_address = 0;

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

    expr::BVExprPtr reg_read(const std::string& name);
    expr::BVExprPtr reg_read(uint64_t offset, size_t size);
    void            reg_write(const std::string& name, expr::BVExprPtr data);
    void            reg_write(uint64_t offset, expr::BVExprPtr data);

    expr::BVExprPtr get_int_param(CallConv cv, uint64_t i);

    void register_linked_function(uint64_t addr, const models::Model* m);
    bool is_linked_function(uint64_t addr);
    const models::Model* get_linked_model(uint64_t addr);
    uint64_t             get_exit_address() const { return m_exit_address; }

    FileSystem& fs() { return *m_fs; }

    Solver&           solver() { return m_solver; }
    expr::BoolExprPtr pi() const;

    void     set_pc(uint64_t pc) { m_pc = pc; }
    uint64_t pc() const { return m_pc; }

    StatePtr clone() const { return std::shared_ptr<State>(new State(*this)); }

    // exited stuff
    bool    exited  = false;
    int32_t retcode = 0;
};

} // namespace naaz::state
