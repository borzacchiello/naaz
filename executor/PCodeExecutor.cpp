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

void PCodeExecutor::handle_symbolic_ip(ExecutionContext& ctx,
                                       expr::BVExprPtr   ip)
{
    auto dests = ctx.state->solver().evaluate_upto(ip, 256);
    if (dests.size() == 256) {
        info("PCodeExecutor")
            << "handle_symbolic_ip(): unconstrained IP" << std::endl;
        ctx.state->exited  = true;
        ctx.state->retcode = 1204;
        ctx.successors.exited.push_back(ctx.state);
    } else {
        for (auto dst : dests) {
            auto successor = ctx.state->clone();
            successor->set_pc(dst.as_u64());
            ctx.successors.active.push_back(successor);
        }
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
            uint32_t low  = op.inputs[1].offset * 8;
            uint32_t high = op.output->size * 8 + low - 1;
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
        case csleigh_CPUI_INT_SEXT: {
            assert(op.output != nullptr && "INT_SEXT: output is NULL");
            assert(op.inputs_count == 1 && "INT_SEXT: inputs_count != 1");
            write_to_varnode(
                ctx, *op.output,
                exprBuilder.mk_sext(resolve_varnode(ctx, op.inputs[0]),
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
        case csleigh_CPUI_INT_OR: {
            assert(op.output != nullptr && "INT_OR: output is NULL");
            assert(op.inputs_count == 2 && "INT_OR: inputs_count != 2");
            expr::BVExprPtr expr =
                exprBuilder.mk_or(resolve_varnode(ctx, op.inputs[0]),
                                  resolve_varnode(ctx, op.inputs[1]));
            write_to_varnode(ctx, *op.output, expr);
            break;
        }
        case csleigh_CPUI_INT_LEFT: {
            assert(op.output != nullptr && "INT_LEFT: output is NULL");
            assert(op.inputs_count == 2 && "INT_LEFT: inputs_count != 2");
            expr::BVExprPtr expr = exprBuilder.mk_shl(
                resolve_varnode(ctx, op.inputs[0]),
                exprBuilder.mk_zext(resolve_varnode(ctx, op.inputs[1]),
                                    op.inputs[0].size * 8));
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
        case csleigh_CPUI_INT_MULT: {
            assert(op.output != nullptr && "INT_MULT: output is NULL");
            assert(op.inputs_count == 2 && "INT_MULT: inputs_count != 2");
            expr::BVExprPtr expr =
                exprBuilder.mk_mul(resolve_varnode(ctx, op.inputs[0]),
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
        case csleigh_CPUI_FLOAT_FLOAT2FLOAT: {
            assert(op.output != nullptr && "FLOAT_FLOAT2FLOAT: output is NULL");
            assert(op.inputs_count == 1 &&
                   "FLOAT_FLOAT2FLOAT: inputs_count != 1");

            auto src_ff = m_lifter->get_float_format(op.inputs[0].size);
            auto dst_ff = m_lifter->get_float_format(op.output->size);
            if (!src_ff || !dst_ff) {
                err("PCodeExecutor")
                    << "FLOAT_FLOAT2FLOAT: invalid conversion (no float format)"
                    << std::endl;
                exit_fail();
            }

            auto src           = resolve_varnode(ctx, op.inputs[0]);
            auto src_fp        = exprBuilder.mk_bv_to_fp(src_ff, src);
            auto converted_src = exprBuilder.mk_fp_convert(src_fp, dst_ff);
            auto res           = exprBuilder.mk_fp_to_bv(converted_src);
            write_to_varnode(ctx, *op.output, res);
            break;
        }
        case csleigh_CPUI_FLOAT_INT2FLOAT: {
            assert(op.output != nullptr && "FLOAT_INT2FLOAT: output is NULL");
            assert(op.inputs_count == 1 &&
                   "FLOAT_INT2FLOAT: inputs_count != 1");

            auto dst_ff = m_lifter->get_float_format(op.output->size);
            if (!dst_ff) {
                err("PCodeExecutor")
                    << "FLOAT_INT2FLOAT: invalid conversion (no float format)"
                    << std::endl;
                exit_fail();
            }

            auto src           = resolve_varnode(ctx, op.inputs[0]);
            auto converted_src = exprBuilder.mk_int_to_fp(src, dst_ff);
            auto res           = exprBuilder.mk_fp_to_bv(converted_src);
            write_to_varnode(ctx, *op.output, res);
            break;
        }
        case csleigh_CPUI_FLOAT_NAN: {
            assert(op.output != nullptr && "FLOAT_NAN: output is NULL");
            assert(op.inputs_count == 1 && "FLOAT_NAN: inputs_count != 1");

            auto src_ff = m_lifter->get_float_format(op.inputs[0].size);
            if (!src_ff) {
                err("PCodeExecutor")
                    << "FLOAT_NAN: invalid conversion (no float format)"
                    << std::endl;
                exit_fail();
            }

            auto src    = resolve_varnode(ctx, op.inputs[0]);
            auto src_fp = exprBuilder.mk_bv_to_fp(src_ff, src);
            auto is_nan = exprBuilder.mk_fp_is_nan(src_fp);
            write_to_varnode(ctx, *op.output, exprBuilder.bool_to_bv(is_nan));
            break;
        }
        case csleigh_CPUI_FLOAT_NEG: {
            assert(op.output != nullptr && "FLOAT_NEG: output is NULL");
            assert(op.inputs_count == 1 && "FLOAT_NEG: inputs_count != 1");

            auto src_ff = m_lifter->get_float_format(op.inputs[0].size);
            if (!src_ff) {
                err("PCodeExecutor")
                    << "FLOAT_NEG: invalid conversion (no float format)"
                    << std::endl;
                exit_fail();
            }

            auto src    = resolve_varnode(ctx, op.inputs[0]);
            auto src_fp = exprBuilder.mk_bv_to_fp(src_ff, src);
            auto res    = exprBuilder.mk_fp_neg(src_fp);
            write_to_varnode(ctx, *op.output, exprBuilder.mk_fp_to_bv(res));
            break;
        }
        case csleigh_CPUI_FLOAT_ADD: {
            assert(op.output != nullptr && "FLOAT_ADD: output is NULL");
            assert(op.inputs_count == 2 && "FLOAT_ADD: inputs_count != 2");

            auto lhs_ff = m_lifter->get_float_format(op.inputs[0].size);
            auto rhs_ff = m_lifter->get_float_format(op.inputs[1].size);
            if (!lhs_ff || !rhs_ff) {
                err("PCodeExecutor")
                    << "FLOAT_ADD: invalid conversion (no float format)"
                    << std::endl;
                exit_fail();
            }

            auto lhs    = resolve_varnode(ctx, op.inputs[0]);
            auto lhs_fp = exprBuilder.mk_bv_to_fp(lhs_ff, lhs);
            auto rhs    = resolve_varnode(ctx, op.inputs[1]);
            auto rhs_fp = exprBuilder.mk_bv_to_fp(rhs_ff, rhs);
            auto res    = exprBuilder.mk_fp_add(lhs_fp, rhs_fp);

            write_to_varnode(ctx, *op.output, exprBuilder.mk_fp_to_bv(res));
            break;
        }
        case csleigh_CPUI_FLOAT_SUB: {
            assert(op.output != nullptr && "FLOAT_SUB: output is NULL");
            assert(op.inputs_count == 2 && "FLOAT_SUB: inputs_count != 2");

            auto lhs_ff = m_lifter->get_float_format(op.inputs[0].size);
            auto rhs_ff = m_lifter->get_float_format(op.inputs[1].size);
            if (!lhs_ff || !rhs_ff) {
                err("PCodeExecutor")
                    << "FLOAT_SUB: invalid conversion (no float format)"
                    << std::endl;
                exit_fail();
            }

            auto lhs    = resolve_varnode(ctx, op.inputs[0]);
            auto lhs_fp = exprBuilder.mk_bv_to_fp(lhs_ff, lhs);
            auto rhs    = resolve_varnode(ctx, op.inputs[1]);
            auto rhs_fp = exprBuilder.mk_bv_to_fp(rhs_ff, rhs);
            auto res    = exprBuilder.mk_fp_sub(lhs_fp, rhs_fp);

            write_to_varnode(ctx, *op.output, exprBuilder.mk_fp_to_bv(res));
            break;
        }
        case csleigh_CPUI_FLOAT_MULT: {
            assert(op.output != nullptr && "FLOAT_MULT: output is NULL");
            assert(op.inputs_count == 2 && "FLOAT_MULT: inputs_count != 2");

            auto lhs_ff = m_lifter->get_float_format(op.inputs[0].size);
            auto rhs_ff = m_lifter->get_float_format(op.inputs[1].size);
            if (!lhs_ff || !rhs_ff) {
                err("PCodeExecutor")
                    << "FLOAT_MULT: invalid conversion (no float format)"
                    << std::endl;
                exit_fail();
            }

            auto lhs    = resolve_varnode(ctx, op.inputs[0]);
            auto lhs_fp = exprBuilder.mk_bv_to_fp(lhs_ff, lhs);
            auto rhs    = resolve_varnode(ctx, op.inputs[1]);
            auto rhs_fp = exprBuilder.mk_bv_to_fp(rhs_ff, rhs);
            auto res    = exprBuilder.mk_fp_mul(lhs_fp, rhs_fp);
            write_to_varnode(ctx, *op.output, exprBuilder.mk_fp_to_bv(res));
            break;
        }
        case csleigh_CPUI_FLOAT_DIV: {
            assert(op.output != nullptr && "FLOAT_DIV: output is NULL");
            assert(op.inputs_count == 2 && "FLOAT_DIV: inputs_count != 2");

            auto lhs_ff = m_lifter->get_float_format(op.inputs[0].size);
            auto rhs_ff = m_lifter->get_float_format(op.inputs[1].size);
            if (!lhs_ff || !rhs_ff) {
                err("PCodeExecutor")
                    << "FLOAT_DIV: invalid conversion (no float format)"
                    << std::endl;
                exit_fail();
            }

            auto lhs    = resolve_varnode(ctx, op.inputs[0]);
            auto lhs_fp = exprBuilder.mk_bv_to_fp(lhs_ff, lhs);
            auto rhs    = resolve_varnode(ctx, op.inputs[1]);
            auto rhs_fp = exprBuilder.mk_bv_to_fp(rhs_ff, rhs);
            auto res    = exprBuilder.mk_fp_div(lhs_fp, rhs_fp);
            write_to_varnode(ctx, *op.output, exprBuilder.mk_fp_to_bv(res));
            break;
        }
        case csleigh_CPUI_FLOAT_EQUAL: {
            assert(op.output != nullptr && "FLOAT_EQUAL: output is NULL");
            assert(op.inputs_count == 2 && "FLOAT_EQUAL: inputs_count != 2");

            auto lhs_ff = m_lifter->get_float_format(op.inputs[0].size);
            auto rhs_ff = m_lifter->get_float_format(op.inputs[1].size);
            if (!lhs_ff || !rhs_ff) {
                err("PCodeExecutor")
                    << "FLOAT_EQUAL: invalid conversion (no float format)"
                    << std::endl;
                exit_fail();
            }

            auto lhs    = resolve_varnode(ctx, op.inputs[0]);
            auto lhs_fp = exprBuilder.mk_bv_to_fp(lhs_ff, lhs);
            auto rhs    = resolve_varnode(ctx, op.inputs[1]);
            auto rhs_fp = exprBuilder.mk_bv_to_fp(rhs_ff, rhs);
            auto res    = exprBuilder.mk_fp_eq(lhs_fp, rhs_fp);
            write_to_varnode(ctx, *op.output, exprBuilder.bool_to_bv(res));
            break;
        }
        case csleigh_CPUI_FLOAT_LESS: {
            assert(op.output != nullptr && "FLOAT_LESS: output is NULL");
            assert(op.inputs_count == 2 && "FLOAT_LESS: inputs_count != 2");

            auto lhs_ff = m_lifter->get_float_format(op.inputs[0].size);
            auto rhs_ff = m_lifter->get_float_format(op.inputs[1].size);
            if (!lhs_ff || !rhs_ff) {
                err("PCodeExecutor")
                    << "FLOAT_LESS: invalid conversion (no float format)"
                    << std::endl;
                exit_fail();
            }

            auto lhs    = resolve_varnode(ctx, op.inputs[0]);
            auto lhs_fp = exprBuilder.mk_bv_to_fp(lhs_ff, lhs);
            auto rhs    = resolve_varnode(ctx, op.inputs[1]);
            auto rhs_fp = exprBuilder.mk_bv_to_fp(rhs_ff, rhs);
            auto res    = exprBuilder.mk_fp_lt(lhs_fp, rhs_fp);
            write_to_varnode(ctx, *op.output, exprBuilder.bool_to_bv(res));
            break;
        }
        case csleigh_CPUI_CALL: {
            assert(op.output == nullptr && "CALL: output is not NULL");
            assert(op.inputs_count == 1 && "CALL: inputs_count != 1");

            uint64_t retaddr = ctx.transl.address.offset + ctx.transl.length;
            ctx.state->register_call(retaddr);

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
            ctx.state = nullptr;
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
            ctx.state = nullptr;
            break;
        }
        case csleigh_CPUI_CBRANCH: {
            // NOTE: a CBRANCH can be either in the middle of a basic block, or
            // at the end.

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
            // std::cout << "cond: " << cond->to_string() << std::endl;
            // std::cout << "pi: " << ctx.state->pi()->to_string() << std::endl;

            state::StatePtr     other_state = ctx.state->clone();
            solver::CheckResult sat_cond =
                other_state->solver().check_sat_and_add_if_sat(cond);
            if (sat_cond == solver::CheckResult::UNKNOWN) {
                info("PCodeExecutor")
                    << "unknown from solver [1] (assuming SAT)" << std::endl;
                sat_cond = solver::CheckResult::SAT;

                // PI could be unsat at a certain point...
                other_state->solver().add(cond);
            }
            solver::CheckResult sat_not_cond =
                ctx.state->solver().check_sat_and_add_if_sat(
                    exprBuilder.mk_not(cond));
            if (sat_not_cond == solver::CheckResult::UNKNOWN) {
                info("PCodeExecutor")
                    << "unknown from solver [2] (assuming SAT)" << std::endl;
                sat_not_cond = solver::CheckResult::SAT;

                // PI could be unsat at a certain point...
                ctx.state->solver().add(exprBuilder.mk_not(cond));
            }

            if (sat_not_cond != solver::CheckResult::SAT) {
                // The fallthrough is NOT satisfiable, if the CBRANCH is in the
                // middle of a basic block, the execution must be aborted
                ctx.state = nullptr;
            }

            if (sat_cond == solver::CheckResult::SAT) {
                other_state->set_pc(dst_addr);
                ctx.successors.active.push_back(other_state);
            }
            break;
        }
        case csleigh_CPUI_BRANCHIND: {
            assert(op.output == nullptr && "BRANCHIND: output is not NULL");
            assert(op.inputs_count == 1 && "BRANCHIND: inputs_count != 1");

            auto dst = resolve_varnode(ctx, op.inputs[0]);
            if (dst->kind() != expr::Expr::Kind::CONST) {
                handle_symbolic_ip(ctx, dst);
            } else {
                auto dst_ =
                    std::static_pointer_cast<const expr::ConstExpr>(dst);
                ctx.state->set_pc(dst_->val().as_u64());
                ctx.successors.active.push_back(ctx.state);
            }
            ctx.state = nullptr;
            break;
        }
        case csleigh_CPUI_CALLIND: {
            assert(op.output == nullptr && "CALLIND: output is not NULL");
            assert(op.inputs_count == 1 && "CALLIND: inputs_count != 1");

            uint64_t retaddr = ctx.transl.address.offset + ctx.transl.length;
            ctx.state->register_call(retaddr);

            auto dst = resolve_varnode(ctx, op.inputs[0]);
            if (dst->kind() != expr::Expr::Kind::CONST) {
                handle_symbolic_ip(ctx, dst);
            } else {
                auto dst_ =
                    std::static_pointer_cast<const expr::ConstExpr>(dst);
                ctx.state->set_pc(dst_->val().as_u64());
                ctx.successors.active.push_back(ctx.state);
            }
            ctx.state = nullptr;
            break;
        }
        case csleigh_CPUI_RETURN: {
            assert(op.output == nullptr && "RETURN: output is not NULL");
            assert(op.inputs_count == 1 && "RETURN: inputs_count != 1");

            ctx.state->register_ret();

            auto dst = resolve_varnode(ctx, op.inputs[0]);
            if (dst->kind() != expr::Expr::Kind::CONST) {
                handle_symbolic_ip(ctx, dst);
            } else {
                auto dst_ =
                    std::static_pointer_cast<const expr::ConstExpr>(dst);
                ctx.state->set_pc(dst_->val().as_u64());
                ctx.successors.active.push_back(ctx.state);
            }
            ctx.state = nullptr;
            break;
        }
        default:
            err("PCodeExecutor") << "Unsupported opcode "
                                 << csleigh_OpCodeName(op.opcode) << std::endl;
            exit_fail();
    }
}

state::StatePtr PCodeExecutor::execute_instruction(state::StatePtr     state,
                                                   csleigh_Translation t,
                                                   ExecutorResult& o_successors)
{
    state::MapMemory tmp_storage(
        "tmp", state::MapMemory::UninitReadBehavior::THROW_ERR);
    ExecutionContext ctx(state, tmp_storage, t, o_successors);

    for (uint32_t i = 0; i < t.ops_count; ++i) {
        if (ctx.state == nullptr)
            break;
        csleigh_PcodeOp op = t.ops[i];
        execute_pcodeop(ctx, op);
    }
    return ctx.state;
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
        state = execute_instruction(state, t, successors);
        if (state == nullptr)
            break;
    }

    if (state != nullptr) {
        // There was a CBRANCH at the end of the basic block, and the
        // fallthrough is SAT. The state is a (fallthrough) successor!
        auto last_instruction = tr->instructions[tr->instructions_count - 1];
        state->set_pc(last_instruction.address.offset +
                      last_instruction.length);
        successors.active.push_back(state);
    }

    return successors;
}

}; // namespace naaz::executor
