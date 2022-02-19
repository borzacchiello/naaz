#pragma once

#include <vector>
#include <memory>

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
        state::StatePtr               state;
        state::MapMemory&             tmp_storage;
        std::vector<state::StatePtr>& successors;

        ExecutionContext(state::StatePtr state_, state::MapMemory& tmp_storage_,
                         std::vector<state::StatePtr>& successors_)
            : state(state_), tmp_storage(tmp_storage_), successors(successors_)
        {
        }
    };

    expr::ExprPtr resolve_varnode(ExecutionContext& ctx, csleigh_Varnode node);
    void          write_to_varnode(ExecutionContext& ctx, csleigh_Varnode node,
                                   expr::ExprPtr value);
    void          execute_pcodeop(ExecutionContext& ctx, csleigh_PcodeOp op);
    void execute_instruction(state::StatePtr state, csleigh_Translation t,
                             std::vector<state::StatePtr>& o_successors);

  public:
    PCodeExecutor(std::shared_ptr<lifter::PCodeLifter> lifter);

    std::vector<state::StatePtr> execute_basic_block(state::StatePtr state);
};

} // namespace naaz::executor
