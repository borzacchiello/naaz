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

expr::BVExprPtr PCodeExecutor::resolve_varnode(ExecutionContext& ctx,
                                               csleigh_Varnode   node)
{
    uint32_t space_id = csleigh_AddrSpace_getId(node.space);
    if (space_id == m_ram_space_id) {
        return ctx.state->read(node.offset, node.size);
    } else if (space_id == m_regs_space_id) {
        return ctx.state->reg_read(node.offset, node.size);
    } else if (space_id == m_const_space_id) {
        return exprBuilder.mk_const(node.offset, node.size * 8);
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
                                     csleigh_Varnode   node,
                                     expr::BVExprPtr   value)
{
    if (node.size * 8 != value->size()) {
        err("PCodeExecutor")
            << "write_to_varnode(): the size of the varnode (" << std::dec
            << node.size * 8 << ") is different from the size of the value ("
            << value->size() << ")" << std::endl;
        exit_fail();
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
        case csleigh_CPUI_INT_EQUAL: {
            assert(op.output != nullptr && "INT_EQUAL: output is NULL");
            assert(op.inputs_count == 2 && "INT_EQUAL: inputs_count != 2");
            expr::BoolExprPtr expr =
                exprBuilder.mk_eq(resolve_varnode(ctx, op.inputs[0]),
                                  resolve_varnode(ctx, op.inputs[1]));
            write_to_varnode(ctx, *op.output, exprBuilder.bool_to_bv(expr));
            break;
        }
        case csleigh_CPUI_INT_NOTEQUAL: {
            assert(op.output != nullptr && "INT_NOTEQUAL: output is NULL");
            assert(op.inputs_count == 2 && "INT_NOTEQUAL: inputs_count != 2");
            expr::BoolExprPtr expr =
                exprBuilder.mk_neq(resolve_varnode(ctx, op.inputs[0]),
                                   resolve_varnode(ctx, op.inputs[1]));
            write_to_varnode(ctx, *op.output, exprBuilder.bool_to_bv(expr));
            break;
        }
        case csleigh_CPUI_INT_LESS: {
            assert(op.output != nullptr && "INT_LESS: output is NULL");
            assert(op.inputs_count == 2 && "INT_LESS: inputs_count != 2");
            expr::BoolExprPtr expr =
                exprBuilder.mk_ult(resolve_varnode(ctx, op.inputs[0]),
                                   resolve_varnode(ctx, op.inputs[1]));
            write_to_varnode(ctx, *op.output, exprBuilder.bool_to_bv(expr));
            break;
        }
        case csleigh_CPUI_INT_SLESS: {
            assert(op.output != nullptr && "INT_SLESS: output is NULL");
            assert(op.inputs_count == 2 && "INT_SLESS: inputs_count != 2");
            expr::BoolExprPtr expr =
                exprBuilder.mk_slt(resolve_varnode(ctx, op.inputs[0]),
                                   resolve_varnode(ctx, op.inputs[1]));
            write_to_varnode(ctx, *op.output, exprBuilder.bool_to_bv(expr));
            break;
        }
        case csleigh_CPUI_BOOL_NEGATE: {
            assert(op.output != nullptr && "BOOL_NEGATE: output is NULL");
            assert(op.inputs_count == 1 && "BOOL_NEGATE: inputs_count != 1");
            write_to_varnode(ctx, *op.output,
                             exprBuilder.bool_to_bv(
                                 exprBuilder.mk_not(exprBuilder.bv_to_bool(
                                     resolve_varnode(ctx, op.inputs[0])))));
            break;
        }
        case csleigh_CPUI_BOOL_AND: {
            assert(op.output != nullptr && "BOOL_AND: output is NULL");
            assert(op.inputs_count == 2 && "BOOL_AND: inputs_count != 2");
            write_to_varnode(
                ctx, *op.output,
                exprBuilder.bool_to_bv(exprBuilder.mk_bool_and(
                    exprBuilder.bv_to_bool(resolve_varnode(ctx, op.inputs[0])),
                    exprBuilder.bv_to_bool(
                        resolve_varnode(ctx, op.inputs[1])))));
            break;
        }
        case csleigh_CPUI_BOOL_OR: {
            assert(op.output != nullptr && "BOOL_OR: output is NULL");
            assert(op.inputs_count == 2 && "BOOL_OR: inputs_count != 2");
            write_to_varnode(
                ctx, *op.output,
                exprBuilder.bool_to_bv(exprBuilder.mk_bool_or(
                    exprBuilder.bv_to_bool(resolve_varnode(ctx, op.inputs[0])),
                    exprBuilder.bv_to_bool(
                        resolve_varnode(ctx, op.inputs[1])))));
            break;
        }
        case csleigh_CPUI_LOAD: {
            assert(op.output != nullptr && "LOAD: output is NULL");
            assert(op.inputs_count == 2 && "LOAD: inputs_count != 2");

            csleigh_Address   addr = {.space  = op.inputs[0].space,
                                      .offset = op.inputs[0].offset};
            csleigh_AddrSpace as   = csleigh_Addr_getSpaceFromConst(&addr);
            if (csleigh_AddrSpace_getId(as) != m_ram_space_id) {
                err("PCodeExecutor")
                    << "execute_pcodeop(): unexpected AddressSpace in LOAD"
                    << std::endl;
                exit_fail();
            }
            auto value = ctx.state->read(resolve_varnode(ctx, op.inputs[1]),
                                         op.output->size);
            write_to_varnode(ctx, *op.output, value);
            break;
        }
        case csleigh_CPUI_STORE: {
            assert(op.output == nullptr && "STORE: output is not NULL");
            assert(op.inputs_count == 3 && "STORE: inputs_count != 3");

            csleigh_Address   addr = {.space  = op.inputs[0].space,
                                      .offset = op.inputs[0].offset};
            csleigh_AddrSpace as   = csleigh_Addr_getSpaceFromConst(&addr);
            if (csleigh_AddrSpace_getId(as) != m_ram_space_id) {
                err("PCodeExecutor")
                    << "execute_pcodeop(): unexpected AddressSpace in STORE"
                    << std::endl;
                exit_fail();
            }
            auto value = resolve_varnode(ctx, op.inputs[2]);
            ctx.state->write(resolve_varnode(ctx, op.inputs[1]), value);
            break;
        }
        case csleigh_CPUI_COPY: {
            assert(op.output != nullptr && "COPY: output is NULL");
            assert(op.inputs_count == 1 && "COPY: inputs_count != 1");
            write_to_varnode(ctx, *op.output,
                             resolve_varnode(ctx, op.inputs[0]));
            break;
        }
        case csleigh_CPUI_SUBPIECE: {
            assert(op.output != nullptr && "SUBPIECE: output is NULL");
            assert(op.inputs_count == 2 && "SUBPIECE: inputs_count != 2");
            uint32_t high = op.output->size * 8 - 1;
            uint32_t low  = op.inputs[1].offset * 8;
            write_to_varnode(
                ctx, *op.output,
                exprBuilder.mk_extract(resolve_varnode(ctx, op.inputs[0]), high,
                                       low));
            break;
        }
        case csleigh_CPUI_INT_ZEXT: {
            assert(op.output != nullptr && "INT_ZEXT: output is NULL");
            assert(op.inputs_count == 1 && "INT_ZEXT: inputs_count != 1");
            write_to_varnode(
                ctx, *op.output,
                exprBuilder.mk_zext(resolve_varnode(ctx, op.inputs[0]),
                                    op.output->size * 8));
            break;
        }
        case csleigh_CPUI_INT_XOR: {
            assert(op.output != nullptr && "INT_XOR: output is NULL");
            assert(op.inputs_count == 2 && "INT_XOR: inputs_count != 2");
            expr::BVExprPtr expr =
                exprBuilder.mk_xor(resolve_varnode(ctx, op.inputs[0]),
                                   resolve_varnode(ctx, op.inputs[1]));
            write_to_varnode(ctx, *op.output, expr);
            break;
        }
        case csleigh_CPUI_INT_AND: {
            assert(op.output != nullptr && "INT_AND: output is NULL");
            assert(op.inputs_count == 2 && "INT_AND: inputs_count != 2");
            expr::BVExprPtr expr =
                exprBuilder.mk_and(resolve_varnode(ctx, op.inputs[0]),
                                   resolve_varnode(ctx, op.inputs[1]));
            write_to_varnode(ctx, *op.output, expr);
            break;
        }
        case csleigh_CPUI_INT_ADD: {
            assert(op.output != nullptr && "INT_ADD: output is NULL");
            assert(op.inputs_count == 2 && "INT_ADD: inputs_count != 2");
            expr::BVExprPtr expr =
                exprBuilder.mk_add(resolve_varnode(ctx, op.inputs[0]),
                                   resolve_varnode(ctx, op.inputs[1]));
            write_to_varnode(ctx, *op.output, expr);
            break;
        }
        case csleigh_CPUI_INT_SUB: {
            assert(op.output != nullptr && "INT_SUB: output is NULL");
            assert(op.inputs_count == 2 && "INT_SUB: inputs_count != 2");
            expr::BVExprPtr expr =
                exprBuilder.mk_sub(resolve_varnode(ctx, op.inputs[0]),
                                   resolve_varnode(ctx, op.inputs[1]));
            write_to_varnode(ctx, *op.output, expr);
            break;
        }
        case csleigh_CPUI_INT_CARRY: {
            assert(op.output != nullptr && "INT_CARRY: output is NULL");
            assert(op.inputs_count == 2 && "INT_CARRY: inputs_count != 2");

            expr::BVExprPtr in1 = resolve_varnode(ctx, op.inputs[0]);
            expr::BVExprPtr in2 = resolve_varnode(ctx, op.inputs[1]);
            expr::BVExprPtr res =
                exprBuilder.bool_to_bv(exprBuilder.mk_uge(in1, in2));
            write_to_varnode(ctx, *op.output, res);
            break;
        }
        case csleigh_CPUI_INT_SBORROW: {
            assert(op.output != nullptr && "INT_SBORROW: output is NULL");
            assert(op.inputs_count == 2 && "INT_SBORROW: inputs_count != 2");

            expr::BVExprPtr in1 = resolve_varnode(ctx, op.inputs[0]);
            expr::BVExprPtr in2 = resolve_varnode(ctx, op.inputs[1]);
            expr::BVExprPtr res = exprBuilder.mk_sub(in1, in2);

            expr::BVExprPtr a = exprBuilder.sign_bit(in1);
            expr::BVExprPtr b = exprBuilder.sign_bit(in2);
            expr::BVExprPtr r = exprBuilder.sign_bit(res);

            a = exprBuilder.mk_xor(a, r);
            r = exprBuilder.mk_xor(r, b);
            r = exprBuilder.mk_xor(r, exprBuilder.mk_const(1, r->size()));
            a = exprBuilder.mk_and(a, r);
            write_to_varnode(ctx, *op.output, exprBuilder.mk_zext(a, 8));
            break;
        }
        case csleigh_CPUI_INT_SCARRY: {
            assert(op.output != nullptr && "INT_SCARRY: output is NULL");
            assert(op.inputs_count == 2 && "INT_SCARRY: inputs_count != 2");

            expr::BVExprPtr in1 = resolve_varnode(ctx, op.inputs[0]);
            expr::BVExprPtr in2 = resolve_varnode(ctx, op.inputs[1]);
            expr::BVExprPtr res = exprBuilder.mk_sub(in1, in2);

            expr::BVExprPtr a = exprBuilder.sign_bit(in1);
            expr::BVExprPtr b = exprBuilder.sign_bit(in2);
            expr::BVExprPtr r = exprBuilder.sign_bit(res);

            r = exprBuilder.mk_xor(r, a);
            a = exprBuilder.mk_xor(a, b);
            a = exprBuilder.mk_xor(a, exprBuilder.mk_const(1, r->size()));
            r = exprBuilder.mk_and(r, a);
            write_to_varnode(ctx, *op.output, exprBuilder.mk_zext(r, 8));
            break;
        }
        case csleigh_CPUI_POPCOUNT: {
            assert(op.output != nullptr && "POPCOUNT: output is NULL");
            assert(op.inputs_count == 1 && "POPCOUNT: inputs_count != 1");

            expr::BVExprPtr inp_expr = resolve_varnode(ctx, op.inputs[0]);
            uint32_t        dst_size = op.output->size * 8;

            expr::BVExprPtr res = exprBuilder.mk_zext(
                exprBuilder.mk_extract(inp_expr, 0, 0), dst_size);
            for (uint32_t i = 1; i < op.inputs[0].size; ++i)
                res = exprBuilder.mk_add(
                    res, exprBuilder.mk_zext(
                             exprBuilder.mk_extract(inp_expr, i, i), dst_size));
            write_to_varnode(ctx, *op.output, res);
            break;
        }
        case csleigh_CPUI_CALL: {
            assert(op.output == nullptr && "CALL: output is not NULL");
            assert(op.inputs_count == 1 && "CALL: inputs_count != 1");

            uint64_t dst_addr;
            if (csleigh_AddrSpace_getId(op.inputs[0].space) ==
                m_lifter->ram_space_id()) {
                dst_addr = op.inputs[0].offset;
            } else {
                err("PCodeExecutor")
                    << "CALL: unexpected space ("
                    << csleigh_AddrSpace_getName(op.inputs[0].space) << ")"
                    << std::endl;
                exit_fail();
            }

            ctx.state->set_pc(dst_addr);
            ctx.successors.active.push_back(ctx.state);
            break;
        }
        case csleigh_CPUI_BRANCH: {
            assert(op.output == nullptr && "BRANCH: output is not NULL");
            assert(op.inputs_count == 1 && "BRANCH: inputs_count != 1");
            assert(csleigh_AddrSpace_getId(op.inputs[0].space) ==
                       m_lifter->ram_space_id() &&
                   "BRANCH: unexpected space");

            uint64_t dst_addr;
            if (csleigh_AddrSpace_getId(op.inputs[0].space) ==
                m_lifter->ram_space_id()) {
                dst_addr = op.inputs[0].offset;
            } else {
                err("PCodeExecutor")
                    << "BRANCH: unexpected space ("
                    << csleigh_AddrSpace_getName(op.inputs[0].space) << ")"
                    << std::endl;
                exit_fail();
            }

            ctx.state->set_pc(dst_addr);
            ctx.successors.active.push_back(ctx.state);
            break;
        }
        case csleigh_CPUI_CBRANCH: {
            assert(op.output == nullptr && "CBRANCH: output is not NULL");
            assert(op.inputs_count == 2 && "CBRANCH: inputs_count != 2");
            assert(csleigh_AddrSpace_getId(op.inputs[0].space) ==
                       m_lifter->ram_space_id() &&
                   "CBRANCH: unexpected space");

            uint64_t dst_addr;
            if (csleigh_AddrSpace_getId(op.inputs[0].space) ==
                m_lifter->ram_space_id()) {
                dst_addr = op.inputs[0].offset;
            } else {
                err("PCodeExecutor")
                    << "CBRANCH: unexpected space ("
                    << csleigh_AddrSpace_getName(op.inputs[0].space) << ")"
                    << std::endl;
                exit_fail();
            }

            expr::BoolExprPtr cond =
                exprBuilder.bv_to_bool(resolve_varnode(ctx, op.inputs[1]));

            state::StatePtr     other_state = ctx.state->clone();
            solver::CheckResult sat_cond = ctx.state->solver().check_sat(cond);
            if (sat_cond == solver::CheckResult::UNKNOWN) {
                err("PCodeExecutor") << "unknown from solver" << std::endl;
                exit_fail();
            }
            solver::CheckResult sat_not_cond =
                other_state->solver().check_sat(exprBuilder.mk_not(cond));
            if (sat_not_cond == solver::CheckResult::UNKNOWN) {
                err("PCodeExecutor") << "unknown from solver" << std::endl;
                exit_fail();
            }

            if (sat_cond == solver::CheckResult::SAT) {
                ctx.state->set_pc(dst_addr);
                ctx.state->solver().add(cond);
                ctx.successors.active.push_back(ctx.state);
            }
            if (sat_not_cond == solver::CheckResult::SAT) {
                other_state->solver().add(exprBuilder.mk_not(cond));
                other_state->set_pc(ctx.transl.address.offset +
                                    ctx.transl.length);
                ctx.successors.active.push_back(other_state);
            }
            break;
        }
        case csleigh_CPUI_BRANCHIND: {
            assert(op.output == nullptr && "BRANCHIND: output is not NULL");
            assert(op.inputs_count == 1 && "BRANCHIND: inputs_count != 1");

            auto dst = resolve_varnode(ctx, op.inputs[0]);
            if (dst->kind() != expr::Expr::Kind::CONST) {
                err("PCodeExecutor")
                    << "FIXME: unhandled symbolic IP (BRANCHIND)" << std::endl;
                exit_fail();
            }
            auto dst_ = std::static_pointer_cast<const expr::ConstExpr>(dst);
            ctx.state->set_pc(dst_->val().as_u64());
            ctx.successors.active.push_back(ctx.state);
            break;
        }
        case csleigh_CPUI_CALLIND: {
            assert(op.output == nullptr && "CALLIND: output is not NULL");
            assert(op.inputs_count == 1 && "CALLIND: inputs_count != 1");

            auto dst = resolve_varnode(ctx, op.inputs[0]);
            if (dst->kind() != expr::Expr::Kind::CONST) {
                err("PCodeExecutor")
                    << "FIXME: unhandled symbolic IP (CALLIND)" << std::endl;
                exit_fail();
            }
            auto dst_ = std::static_pointer_cast<const expr::ConstExpr>(dst);
            ctx.state->set_pc(dst_->val().as_u64());
            ctx.successors.active.push_back(ctx.state);
            break;
        }
        case csleigh_CPUI_RETURN: {
            assert(op.output == nullptr && "RETURN: output is not NULL");
            assert(op.inputs_count == 1 && "RETURN: inputs_count != 1");

            auto dst = resolve_varnode(ctx, op.inputs[0]);
            if (dst->kind() != expr::Expr::Kind::CONST) {
                err("PCodeExecutor")
                    << "FIXME: unhandled symbolic IP (RETURN)" << std::endl;
                exit_fail();
            }
            auto dst_ = std::static_pointer_cast<const expr::ConstExpr>(dst);
            ctx.state->set_pc(dst_->val().as_u64());
            ctx.successors.active.push_back(ctx.state);
            break;
        }
        default:
            err("PCodeExecutor") << "Unsupported opcode "
                                 << csleigh_OpCodeName(op.opcode) << std::endl;
            exit_fail();
    }
}

void PCodeExecutor::execute_instruction(state::StatePtr     state,
                                        csleigh_Translation t,
                                        ExecutorResult&     o_successors)
{
    state::MapMemory tmp_storage(
        "tmp", state::MapMemory::UninitReadBehavior::THROW_ERR);
    ExecutionContext ctx(state, tmp_storage, t, o_successors);

    for (uint32_t i = 0; i < t.ops_count; ++i) {
        csleigh_PcodeOp op = t.ops[i];
        execute_pcodeop(ctx, op);
    }
}

ExecutorResult PCodeExecutor::execute_basic_block(state::StatePtr state)
{
    ExecutorResult successors;

    if (state->is_linked_function(state->pc())) {
        auto model = state->get_linked_model(state->pc());
        model->exec(state, successors);
        return successors;
    }

    uint8_t* data;
    uint64_t size;

    if (!state->get_code_at(state->pc(), &data, &size)) {
        err("PCodeExecutor")
            << "execute_basic_block(): unable to fetch bytes at 0x" << std::hex
            << state->pc() << std::endl;
        exit_fail();
    }

    const auto block = m_lifter->lift(state->pc(), data, size);
    const csleigh_TranslationResult* tr = block->transl();

    // block->pp();

    for (uint32_t i = 0; i < tr->instructions_count; ++i) {
        csleigh_Translation t = tr->instructions[i];
        state->set_pc(t.address.offset);
        execute_instruction(state, t, successors);
    }

    return successors;
}

}; // namespace naaz::executor
