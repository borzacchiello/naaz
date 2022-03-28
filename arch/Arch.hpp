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

    virtual expr::BVExprPtr get_int_param(CallConv cv, state::State& s,
                                          uint32_t i) const             = 0;
    virtual void set_int_param(CallConv cv, state::State& s, uint32_t i,
                               expr::BVExprPtr val) const               = 0;
    virtual void set_return_int_value(CallConv cv, state::State& s,
                                      expr::BVExprPtr val) const        = 0;
    virtual expr::BVExprPtr get_return_int_value(CallConv      cv,
                                                 state::State& s) const = 0;

    virtual const std::string& description() const = 0;
};

namespace arch
{

class x86_64 final : public naaz::Arch
{
    uint64_t stack_ptr     = 0xcc000000000UL;
    uint64_t heap_ptr      = 0xdd000000000UL;
    uint64_t ext_func_base = 0x5000000UL;
    uint64_t fs_base_ptr   = 0xff000000000UL;

    x86_64() {}

  public:
    virtual std::filesystem::path getSleighSLA() const;
    virtual std::filesystem::path getSleighPSPEC() const;
    virtual Endianess endianess() const { return Endianess::LITTLE; }
    virtual uint64_t  ptr_size() const { return 64UL; };

    virtual uint64_t get_stack_ptr() const { return stack_ptr; }
    virtual uint64_t get_heap_ptr() const { return heap_ptr; }
    virtual uint64_t get_ext_func_base() const { return ext_func_base; }

    virtual void init_state(state::State& s) const;
    virtual void handle_return(state::StatePtr           s,
                               executor::ExecutorResult& o_successors) const;
    virtual void set_return(state::StatePtr s, expr::BVExprPtr addr) const;

    virtual expr::BVExprPtr get_int_param(CallConv cv, state::State& s,
                                          uint32_t i) const;
    virtual void set_int_param(CallConv cv, state::State& s, uint32_t i,
                               expr::BVExprPtr val) const;
    virtual void set_return_int_value(CallConv cv, state::State& s,
                                      expr::BVExprPtr val) const;
    virtual expr::BVExprPtr get_return_int_value(CallConv      cv,
                                                 state::State& s) const;

    virtual const std::string& description() const;

    static const x86_64& The()
    {
        static x86_64 singleton;
        return singleton;
    }
};

} // namespace arch
} // namespace naaz
