#include <cassert>

#include "PCodeExecutor.hpp"

#include "../expr/ExprBuilder.hpp"
#include "../util/ioutil.hpp"

#define exprBuilder naaz::expr::ExprBuilder::The()

namespace naaz::executor
{

PCodeExecutor::PCodeExecutor(std::shared_ptr<lifter::PCodeLifter> lifter)
    : m_lifter(lifter)
{
    m_ram_space_id   = lifter->ram_space_id();
    m_regs_space_id  = lifter->regs_space_id();
    m_const_space_id = lifter->const_space_id();
    m_tmp_space_id   = lifter->tmp_space_id();
}

expr::ExprPtr PCodeExecutor::resolve_varnode(ExecutionContext& ctx,
                                             csleigh_Varnode   node)
{
    uint32_t space_id = csleigh_AddrSpace_getId(node.space);
    if (space_id == m_ram_space_id) {
        return ctx.state->read(node.offset, node.size);
    } else if (space_id == m_regs_space_id) {
        return ctx.state->reg_read(node.offset, node.size);
    } else if (space_id == m_const_space_id) {
        return exprBuilder.mk_const(node.offset, node.size);
    } else if (space_id == m_tmp_space_id) {
        return ctx.tmp_storage.read(node.offset, node.size);
    } else {
        err("PCodeExecutor")
            << "resolve_varnode(): unknown space '"
            << csleigh_AddrSpace_getName(node.space) << "'" << std::endl;
        exit_fail();
    }
}

void PCodeExecutor::write_to_varnode(ExecutionContext& ctx,
                                     csleigh_Varnode node, expr::ExprPtr value)
{
    if (node.size != value->size()) {
        err("PCodeExecutor")
            << "write_to_varnode(): the size of the varnode (" << std::dec
            << node.size << ") is different from the size of the value ("
            << value->size() << ")" << std::endl;
        exit_fail();
    }

    if (node.size == 1) {
        // extend bools to byte
        value = exprBuilder.mk_zext(value, 8);
    }

    uint32_t space_id = csleigh_AddrSpace_getId(node.space);
    if (space_id == m_ram_space_id) {
        ctx.state->write(node.offset, value);
    } else if (space_id == m_regs_space_id) {
        ctx.state->reg_write(node.offset, value);
    } else if (space_id == m_tmp_space_id) {
        return ctx.tmp_storage.write(node.offset, value);
    } else {
        err("PCodeExecutor")
            << "write_to_varnode(): unknown space '"
            << csleigh_AddrSpace_getName(node.space) << "'" << std::endl;
        exit_fail();
    }
}

void PCodeExecutor::execute_pcodeop(ExecutionContext& ctx, csleigh_PcodeOp op)
{
    switch (op.opcode) {
        case csleigh_CPUI_COPY: {
            assert(op.output != nullptr && "CPUI_COPY: output is NULL");
            assert(op.inputs_count == 1 && "CPUI_COPY: inputs_count != 1");
            write_to_varnode(ctx, *op.output,
                             resolve_varnode(ctx, op.inputs[0]));
            break;
        }
        case csleigh_CPUI_INT_LESS: {
            assert(op.output != nullptr && "CPUI_INT_LESS: output is NULL");
            assert(op.inputs_count == 2 && "CPUI_INT_LESS: inputs_count != 1");
            expr::ExprPtr expr =
                exprBuilder.mk_ult(resolve_varnode(ctx, op.inputs[0]),
                                   resolve_varnode(ctx, op.inputs[1]));
            write_to_varnode(ctx, *op.output, expr);
            break;
        }
        case csleigh_CPUI_INT_SLESS: {
            assert(op.output != nullptr && "CPUI_INT_SLESS: output is NULL");
            assert(op.inputs_count == 2 && "CPUI_INT_SLESS: inputs_count != 1");
            expr::ExprPtr expr =
                exprBuilder.mk_slt(resolve_varnode(ctx, op.inputs[0]),
                                   resolve_varnode(ctx, op.inputs[1]));
            write_to_varnode(ctx, *op.output, expr);
            break;
        }
        default:
            err("PCodeExecutor") << "Unsupported opcode "
                                 << csleigh_OpCodeName(op.opcode) << std::endl;
            exit_fail();
    }
}

void PCodeExecutor::execute_instruction(
    state::StatePtr state, csleigh_Translation t,
    std::vector<state::StatePtr>& o_successors)
{
    state::MapMemory tmp_storage(
        state::MapMemory::UninitReadBehavior::THROW_ERR);
    ExecutionContext ctx(state, tmp_storage, o_successors);

    for (uint32_t i = 0; i < t.ops_count; ++i) {
        csleigh_PcodeOp op = t.ops[i];
        execute_pcodeop(ctx, op);
    }
}

std::vector<state::StatePtr>
PCodeExecutor::execute_basic_block(state::StatePtr state)
{
    uint8_t* data;
    uint64_t size;

    if (!state->get_code_at(state->pc(), &data, &size)) {
        err("PCodeExecutor")
            << "execute_basic_block(): unable to fetch bytes at 0x" << std::hex
            << state->pc() << std::endl;
        exit_fail();
    }

    const auto& block = m_lifter->lift(state->pc(), data, size);
    const csleigh_TranslationResult* tr = block->transl();

    std::vector<state::StatePtr> successors;

    for (uint32_t i = 0; i < tr->instructions_count; ++i) {
        csleigh_Translation t = tr->instructions[i];
        state->set_pc(t.address.offset);
        execute_instruction(state, t, successors);
    }

    return successors;
}

}; // namespace naaz::executor
