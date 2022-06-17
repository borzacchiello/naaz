#pragma once

#include <memory>
#include <vector>
#include <cstdlib>
#include <filesystem>

#include "../models/Model.hpp"
#include "../executor/Executor.hpp"

namespace naaz
{

namespace state
{
class State;
typedef std::shared_ptr<State> StatePtr;
} // namespace state

namespace expr
{
class BVExpr;
typedef std::shared_ptr<const BVExpr> BVExprPtr;
} // namespace expr

enum Endianess { LITTLE, BIG };

class Arch
{
  protected:
    static std::filesystem::path getSleighProcessorDir();

  public:
    virtual std::filesystem::path getSleighSLA() const      = 0;
    virtual std::filesystem::path getSleighPSPEC() const    = 0;
    virtual Endianess             endianess() const         = 0;
    virtual uint64_t              ptr_size() const          = 0;
    virtual uint64_t              get_stack_ptr() const     = 0;
    virtual uint64_t              get_heap_ptr() const      = 0;
    virtual uint64_t              get_ext_func_base() const = 0;

    virtual void init_state(state::State& s) const = 0;
    virtual void
                 handle_return(state::StatePtr           s,
                               executor::ExecutorResult& o_successors) const = 0;
    virtual void set_return(state::StatePtr s, expr::BVExprPtr addr) const = 0;
    virtual void set_return(state::StatePtr s, uint64_t addr) const;

    virtual const std::string& get_stack_ptr_reg() const              = 0;
    virtual int                get_syscall_num(state::State& s) const = 0;
    virtual expr::BVExprPtr    get_syscall_param(state::State& s,
                                                 uint32_t      i) const    = 0;
    virtual void               set_syscall_return_value(state::State&   s,
                                                        expr::BVExprPtr val) const = 0;
    virtual expr::BVExprPtr    get_int_param(CallConv cv, state::State& s,
                                             uint32_t i) const = 0;
    virtual void               set_int_params(CallConv cv, state::State& s,
                                              std::vector<expr::BVExprPtr> values) const = 0;
    virtual void            set_return_int_value(CallConv cv, state::State& s,
                                                 expr::BVExprPtr val) const = 0;
    virtual expr::BVExprPtr get_return_int_value(CallConv      cv,
                                                 state::State& s) const     = 0;

    virtual const std::string& description() const = 0;
};

} // namespace naaz
