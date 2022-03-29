#pragma once

#include <vector>
#include <memory>

#include "Executor.hpp"
#include "../lifter/PCodeLifter.hpp"
#include "../state/MapMemory.hpp"
#include "../state/State.hpp"
#include "../expr/Expr.hpp"

namespace naaz::executor
{

class PCodeExecutor
{
    std::shared_ptr<lifter::PCodeLifter> m_lifter;

    uint32_t m_ram_space_id;
    uint32_t m_regs_space_id;
    uint32_t m_const_space_id;
    uint32_t m_tmp_space_id;

    struct ExecutionContext {
        state::StatePtr      state;
        state::MapMemory&    tmp_storage;
        csleigh_Translation& transl;
        ExecutorResult&      successors;

        ExecutionContext(state::StatePtr state_, state::MapMemory& tmp_storage_,
                         csleigh_Translation& transl_,
                         ExecutorResult&      successors_)
            : state(state_), tmp_storage(tmp_storage_), transl(transl_),
              successors(successors_)
        {
        }
    };

    void handle_symbolic_ip(ExecutionContext& ctx, expr::BVExprPtr ip);
    expr::BVExprPtr resolve_varnode(ExecutionContext& ctx,
                                    csleigh_Varnode   node);
    void write_to_varnode(ExecutionContext& ctx, csleigh_Varnode node,
                          expr::BVExprPtr value);
    void execute_pcodeop(ExecutionContext& ctx, csleigh_PcodeOp op);
    state::StatePtr execute_instruction(state::StatePtr     state,
                                        csleigh_Translation t,
                                        ExecutorResult&     o_successors);

  public:
    PCodeExecutor(std::shared_ptr<lifter::PCodeLifter> lifter);

    ExecutorResult execute_basic_block(state::StatePtr state);
};

} // namespace naaz::executor
