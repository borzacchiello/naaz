#pragma once

#include "Arch.hpp"

namespace naaz::arch
{

class x86_64 final : public naaz::Arch
{
    uint64_t stack_ptr     = 0xcc000000000UL;
    uint64_t heap_ptr      = 0xdd000000000UL;
    uint64_t ext_func_base = 0x5000000UL;
    uint64_t fs_base_ptr   = 0xff000000000UL;

    x86_64() {}

    expr::BVExprPtr stack_pop(state::State& s) const;
    void            stack_push(state::State& s, expr::BVExprPtr val) const;
    expr::BVExprPtr stack_pop(state::StatePtr s) const { return stack_pop(*s); }
    void            stack_push(state::StatePtr s, expr::BVExprPtr val) const
    {
        stack_push(*s, val);
    }

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

    virtual const std::string& get_stack_ptr_reg() const;
    virtual expr::BVExprPtr    get_int_param(CallConv cv, state::State& s,
                                             uint32_t i) const;
    virtual void               set_int_params(CallConv cv, state::State& s,
                                              std::vector<expr::BVExprPtr> values) const;
    virtual void            set_return_int_value(CallConv cv, state::State& s,
                                                 expr::BVExprPtr val) const;
    virtual expr::BVExprPtr get_return_int_value(CallConv      cv,
                                                 state::State& s) const;

    virtual const std::string& description() const;

    static const x86_64& The()
    {
        static x86_64 singleton;
        return singleton;
    }
}; // namespace arch

} // namespace naaz::arch
