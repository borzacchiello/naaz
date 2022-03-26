#include <string>

#include "../util/ioutil.hpp"
#include "../util/strutil.hpp"

#include "util.hpp"
#include "ExprBuilder.hpp"

#define exprBuilder naaz::expr::ExprBuilder::The()

namespace naaz::expr
{

static ExprPtr evaluate_inner(ExprPtr                            e,
                              const std::map<uint32_t, BVConst>& assignments,
                              bool                         model_completion,
                              std::map<uint64_t, ExprPtr>& cache)
{
    if (cache.contains((uint64_t)e.get()))
        return cache.at((uint64_t)e.get());

    ExprPtr res;
    switch (e->kind()) {
        case Expr::Kind::SYM: {
            auto e_ = std::static_pointer_cast<const SymExpr>(e);
            if (assignments.contains(e_->id()))
                res = exprBuilder.mk_const(assignments.at(e_->id()));
            else if (model_completion)
                res = exprBuilder.mk_const(0, e_->size());
            else
                res = e;
            break;
        }
        case Expr::Kind::CONST:
        case Expr::Kind::BOOL_CONST:
            res = e;
            break;
        case Expr::Kind::EXTRACT: {
            auto e_     = std::static_pointer_cast<const ExtractExpr>(e);
            auto eval_e = std::static_pointer_cast<const BVExpr>(evaluate_inner(
                e_->expr(), assignments, model_completion, cache));
            res         = exprBuilder.mk_extract(eval_e, e_->high(), e_->low());
            break;
        }
        case Expr::Kind::CONCAT: {
            auto e_ = std::static_pointer_cast<const ConcatExpr>(e);
            auto eval_expr =
                std::static_pointer_cast<const BVExpr>(evaluate_inner(
                    e_->els().at(0), assignments, model_completion, cache));
            for (uint32_t i = 1; i < e_->els().size(); ++i)
                eval_expr = exprBuilder.mk_concat(
                    eval_expr, std::static_pointer_cast<const BVExpr>(
                                   evaluate_inner(e_->els().at(i), assignments,
                                                  model_completion, cache)));
            res = eval_expr;
            break;
        }
        case Expr::Kind::ZEXT: {
            auto e_     = std::static_pointer_cast<const ZextExpr>(e);
            auto eval_e = std::static_pointer_cast<const BVExpr>(evaluate_inner(
                e_->expr(), assignments, model_completion, cache));
            res         = exprBuilder.mk_zext(eval_e, e_->size());
            break;
        }
        case Expr::Kind::SEXT: {
            auto e_     = std::static_pointer_cast<const SextExpr>(e);
            auto eval_e = std::static_pointer_cast<const BVExpr>(evaluate_inner(
                e_->expr(), assignments, model_completion, cache));
            res         = exprBuilder.mk_sext(eval_e, e_->size());
            break;
        }
        case Expr::Kind::ITE: {
            auto e_ = std::static_pointer_cast<const ITEExpr>(e);
            auto eval_guard =
                std::static_pointer_cast<const BoolExpr>(evaluate_inner(
                    e_->guard(), assignments, model_completion, cache));
            auto eval_iftrue =
                std::static_pointer_cast<const BVExpr>(evaluate_inner(
                    e_->iftrue(), assignments, model_completion, cache));
            auto eval_iffalse =
                std::static_pointer_cast<const BVExpr>(evaluate_inner(
                    e_->iffalse(), assignments, model_completion, cache));
            res = exprBuilder.mk_ite(eval_guard, eval_iftrue, eval_iffalse);
            break;
        }
        case Expr::Kind::SHL: {
            auto e_ = std::static_pointer_cast<const ShlExpr>(e);
            auto eval_expr =
                std::static_pointer_cast<const BVExpr>(evaluate_inner(
                    e_->expr(), assignments, model_completion, cache));
            auto eval_val =
                std::static_pointer_cast<const BVExpr>(evaluate_inner(
                    e_->val(), assignments, model_completion, cache));
            res = exprBuilder.mk_shl(eval_expr, eval_val);
            break;
        }
        case Expr::Kind::LSHR: {
            auto e_ = std::static_pointer_cast<const LShrExpr>(e);
            auto eval_expr =
                std::static_pointer_cast<const BVExpr>(evaluate_inner(
                    e_->expr(), assignments, model_completion, cache));
            auto eval_val =
                std::static_pointer_cast<const BVExpr>(evaluate_inner(
                    e_->val(), assignments, model_completion, cache));
            res = exprBuilder.mk_lshr(eval_expr, eval_val);
            break;
        }
        case Expr::Kind::ASHR: {
            auto e_ = std::static_pointer_cast<const AShrExpr>(e);
            auto eval_expr =
                std::static_pointer_cast<const BVExpr>(evaluate_inner(
                    e_->expr(), assignments, model_completion, cache));
            auto eval_val =
                std::static_pointer_cast<const BVExpr>(evaluate_inner(
                    e_->val(), assignments, model_completion, cache));
            res = exprBuilder.mk_ashr(eval_expr, eval_val);
            break;
        }
        case Expr::Kind::NEG: {
            auto e_ = std::static_pointer_cast<const NegExpr>(e);
            auto eval_expr =
                std::static_pointer_cast<const BVExpr>(evaluate_inner(
                    e_->expr(), assignments, model_completion, cache));
            res = exprBuilder.mk_neg(eval_expr);
            break;
        }
        case Expr::Kind::NOT: {
            auto e_ = std::static_pointer_cast<const NotExpr>(e);
            auto eval_expr =
                std::static_pointer_cast<const BVExpr>(evaluate_inner(
                    e_->expr(), assignments, model_completion, cache));
            res = exprBuilder.mk_not(eval_expr);
            break;
        }
        case Expr::Kind::AND: {
            auto e_ = std::static_pointer_cast<const AndExpr>(e);
            auto eval_expr =
                std::static_pointer_cast<const BVExpr>(evaluate_inner(
                    e_->els().at(0), assignments, model_completion, cache));
            for (uint32_t i = 1; i < e_->els().size(); ++i)
                eval_expr = exprBuilder.mk_and(
                    eval_expr, std::static_pointer_cast<const BVExpr>(
                                   evaluate_inner(e_->els().at(i), assignments,
                                                  model_completion, cache)));
            res = eval_expr;
            break;
        }
        case Expr::Kind::OR: {
            auto e_ = std::static_pointer_cast<const OrExpr>(e);
            auto eval_expr =
                std::static_pointer_cast<const BVExpr>(evaluate_inner(
                    e_->els().at(0), assignments, model_completion, cache));
            for (uint32_t i = 1; i < e_->els().size(); ++i)
                eval_expr = exprBuilder.mk_or(
                    eval_expr, std::static_pointer_cast<const BVExpr>(
                                   evaluate_inner(e_->els().at(i), assignments,
                                                  model_completion, cache)));
            res = eval_expr;
            break;
        }
        case Expr::Kind::XOR: {
            auto e_ = std::static_pointer_cast<const XorExpr>(e);
            auto eval_expr =
                std::static_pointer_cast<const BVExpr>(evaluate_inner(
                    e_->els().at(0), assignments, model_completion, cache));
            for (uint32_t i = 1; i < e_->els().size(); ++i)
                eval_expr = exprBuilder.mk_xor(
                    eval_expr, std::static_pointer_cast<const BVExpr>(
                                   evaluate_inner(e_->els().at(i), assignments,
                                                  model_completion, cache)));
            res = eval_expr;
            break;
        }
        case Expr::Kind::ADD: {
            auto e_ = std::static_pointer_cast<const AddExpr>(e);
            auto eval_expr =
                std::static_pointer_cast<const BVExpr>(evaluate_inner(
                    e_->addends().at(0), assignments, model_completion, cache));
            for (uint32_t i = 1; i < e_->addends().size(); ++i)
                eval_expr = exprBuilder.mk_add(
                    eval_expr,
                    std::static_pointer_cast<const BVExpr>(
                        evaluate_inner(e_->addends().at(i), assignments,
                                       model_completion, cache)));
            res = eval_expr;
            break;
        }
        case Expr::Kind::MUL: {
            auto e_ = std::static_pointer_cast<const MulExpr>(e);
            auto eval_expr =
                std::static_pointer_cast<const BVExpr>(evaluate_inner(
                    e_->els().at(0), assignments, model_completion, cache));
            for (uint32_t i = 1; i < e_->els().size(); ++i)
                eval_expr = exprBuilder.mk_mul(
                    eval_expr, std::static_pointer_cast<const BVExpr>(
                                   evaluate_inner(e_->els().at(i), assignments,
                                                  model_completion, cache)));
            res = eval_expr;
            break;
        }
        case Expr::Kind::SDIV: {
            auto e_ = std::static_pointer_cast<const SDivExpr>(e);
            res     = exprBuilder.mk_sdiv(
                    std::static_pointer_cast<const BVExpr>(evaluate_inner(
                        e_->lhs(), assignments, model_completion, cache)),
                    std::static_pointer_cast<const BVExpr>(evaluate_inner(
                        e_->rhs(), assignments, model_completion, cache)));
            break;
        }
        case Expr::Kind::UDIV: {
            auto e_ = std::static_pointer_cast<const UDivExpr>(e);
            res     = exprBuilder.mk_udiv(
                    std::static_pointer_cast<const BVExpr>(evaluate_inner(
                        e_->lhs(), assignments, model_completion, cache)),
                    std::static_pointer_cast<const BVExpr>(evaluate_inner(
                        e_->rhs(), assignments, model_completion, cache)));
            break;
        }
        case Expr::Kind::SREM: {
            auto e_ = std::static_pointer_cast<const SRemExpr>(e);
            res     = exprBuilder.mk_srem(
                    std::static_pointer_cast<const BVExpr>(evaluate_inner(
                        e_->lhs(), assignments, model_completion, cache)),
                    std::static_pointer_cast<const BVExpr>(evaluate_inner(
                        e_->rhs(), assignments, model_completion, cache)));
            break;
        }
        case Expr::Kind::UREM: {
            auto e_ = std::static_pointer_cast<const URemExpr>(e);
            res     = exprBuilder.mk_urem(
                    std::static_pointer_cast<const BVExpr>(evaluate_inner(
                        e_->lhs(), assignments, model_completion, cache)),
                    std::static_pointer_cast<const BVExpr>(evaluate_inner(
                        e_->rhs(), assignments, model_completion, cache)));
            break;
        }
        case Expr::Kind::BOOL_NOT: {
            auto e_ = std::static_pointer_cast<const BoolNotExpr>(e);
            auto eval_expr =
                std::static_pointer_cast<const BoolExpr>(evaluate_inner(
                    e_->expr(), assignments, model_completion, cache));
            res = exprBuilder.mk_not(eval_expr);
            break;
        }
        case Expr::Kind::ULT: {
            auto e_ = std::static_pointer_cast<const UltExpr>(e);
            auto eval_lhs =
                std::static_pointer_cast<const BVExpr>(evaluate_inner(
                    e_->lhs(), assignments, model_completion, cache));
            auto eval_rhs =
                std::static_pointer_cast<const BVExpr>(evaluate_inner(
                    e_->rhs(), assignments, model_completion, cache));
            res = exprBuilder.mk_ult(eval_lhs, eval_rhs);
            break;
        }
        case Expr::Kind::ULE: {
            auto e_ = std::static_pointer_cast<const UleExpr>(e);
            auto eval_lhs =
                std::static_pointer_cast<const BVExpr>(evaluate_inner(
                    e_->lhs(), assignments, model_completion, cache));
            auto eval_rhs =
                std::static_pointer_cast<const BVExpr>(evaluate_inner(
                    e_->rhs(), assignments, model_completion, cache));
            res = exprBuilder.mk_ule(eval_lhs, eval_rhs);
            break;
        }
        case Expr::Kind::UGT: {
            auto e_ = std::static_pointer_cast<const UgtExpr>(e);
            auto eval_lhs =
                std::static_pointer_cast<const BVExpr>(evaluate_inner(
                    e_->lhs(), assignments, model_completion, cache));
            auto eval_rhs =
                std::static_pointer_cast<const BVExpr>(evaluate_inner(
                    e_->rhs(), assignments, model_completion, cache));
            res = exprBuilder.mk_ugt(eval_lhs, eval_rhs);
            break;
        }
        case Expr::Kind::UGE: {
            auto e_ = std::static_pointer_cast<const UgeExpr>(e);
            auto eval_lhs =
                std::static_pointer_cast<const BVExpr>(evaluate_inner(
                    e_->lhs(), assignments, model_completion, cache));
            auto eval_rhs =
                std::static_pointer_cast<const BVExpr>(evaluate_inner(
                    e_->rhs(), assignments, model_completion, cache));
            res = exprBuilder.mk_uge(eval_lhs, eval_rhs);
            break;
        }
        case Expr::Kind::SLT: {
            auto e_ = std::static_pointer_cast<const SltExpr>(e);
            auto eval_lhs =
                std::static_pointer_cast<const BVExpr>(evaluate_inner(
                    e_->lhs(), assignments, model_completion, cache));
            auto eval_rhs =
                std::static_pointer_cast<const BVExpr>(evaluate_inner(
                    e_->rhs(), assignments, model_completion, cache));
            res = exprBuilder.mk_slt(eval_lhs, eval_rhs);
            break;
        }
        case Expr::Kind::SLE: {
            auto e_ = std::static_pointer_cast<const SleExpr>(e);
            auto eval_lhs =
                std::static_pointer_cast<const BVExpr>(evaluate_inner(
                    e_->lhs(), assignments, model_completion, cache));
            auto eval_rhs =
                std::static_pointer_cast<const BVExpr>(evaluate_inner(
                    e_->rhs(), assignments, model_completion, cache));
            res = exprBuilder.mk_sle(eval_lhs, eval_rhs);
            break;
        }
        case Expr::Kind::SGT: {
            auto e_ = std::static_pointer_cast<const SgtExpr>(e);
            auto eval_lhs =
                std::static_pointer_cast<const BVExpr>(evaluate_inner(
                    e_->lhs(), assignments, model_completion, cache));
            auto eval_rhs =
                std::static_pointer_cast<const BVExpr>(evaluate_inner(
                    e_->rhs(), assignments, model_completion, cache));
            res = exprBuilder.mk_sgt(eval_lhs, eval_rhs);
            break;
        }
        case Expr::Kind::SGE: {
            auto e_ = std::static_pointer_cast<const SgeExpr>(e);
            auto eval_lhs =
                std::static_pointer_cast<const BVExpr>(evaluate_inner(
                    e_->lhs(), assignments, model_completion, cache));
            auto eval_rhs =
                std::static_pointer_cast<const BVExpr>(evaluate_inner(
                    e_->rhs(), assignments, model_completion, cache));
            res = exprBuilder.mk_sge(eval_lhs, eval_rhs);
            break;
        }
        case Expr::Kind::EQ: {
            auto e_ = std::static_pointer_cast<const EqExpr>(e);
            auto eval_lhs =
                std::static_pointer_cast<const BVExpr>(evaluate_inner(
                    e_->lhs(), assignments, model_completion, cache));
            auto eval_rhs =
                std::static_pointer_cast<const BVExpr>(evaluate_inner(
                    e_->rhs(), assignments, model_completion, cache));
            res = exprBuilder.mk_eq(eval_lhs, eval_rhs);
            break;
        }
        case Expr::Kind::BOOL_AND: {
            auto e_ = std::static_pointer_cast<const BoolAndExpr>(e);
            auto eval_expr =
                std::static_pointer_cast<const BoolExpr>(evaluate_inner(
                    e_->exprs().at(0), assignments, model_completion, cache));
            for (uint32_t i = 1; i < e_->exprs().size(); ++i)
                eval_expr = exprBuilder.mk_bool_and(
                    eval_expr,
                    std::static_pointer_cast<const BoolExpr>(
                        evaluate_inner(e_->exprs().at(i), assignments,
                                       model_completion, cache)));
            res = eval_expr;
            break;
        }
        case Expr::Kind::BOOL_OR: {
            auto e_ = std::static_pointer_cast<const BoolOrExpr>(e);
            auto eval_expr =
                std::static_pointer_cast<const BoolExpr>(evaluate_inner(
                    e_->exprs().at(0), assignments, model_completion, cache));
            for (uint32_t i = 1; i < e_->exprs().size(); ++i)
                eval_expr = exprBuilder.mk_bool_or(
                    eval_expr,
                    std::static_pointer_cast<const BoolExpr>(
                        evaluate_inner(e_->exprs().at(i), assignments,
                                       model_completion, cache)));
            res = eval_expr;
            break;
        }
        default:
            err("expr::evaluate_inner")
                << "unexpected kind " << e->kind() << std::endl;
            exit_fail();
    }

    cache[(uint64_t)e.get()] = res;
    return res;
}

ExprPtr evaluate(ExprPtr e, const std::map<uint32_t, BVConst>& assignments,
                 bool model_completion)
{
    std::map<uint64_t, ExprPtr> cache;
    auto res = evaluate_inner(e, assignments, model_completion, cache);
    return res;
}

static std::string to_string_inner(ExprPtr                         e,
                                   std::map<ExprPtr, std::string>& cache)
{
    if (cache.contains(e))
        return cache.at(e);

    std::string res;
    switch (e->kind()) {
        case Expr::Kind::SYM: {
            auto e_ = std::static_pointer_cast<const SymExpr>(e);
            res     = e_->name();
            break;
        }
        case Expr::Kind::CONST: {
            auto e_ = std::static_pointer_cast<const ConstExpr>(e);
            res     = e_->val().to_string(true);
            break;
        }
        case Expr::Kind::BOOL_CONST: {
            auto e_ = std::static_pointer_cast<const BoolConst>(e);
            res     = e_->is_true() ? "true" : "false";
            break;
        }
        case Expr::Kind::EXTRACT: {
            auto e_ = std::static_pointer_cast<const ExtractExpr>(e);
            res     = string_format("%s[%u:%u]",
                                    to_string_inner(e_->expr(), cache).c_str(),
                                    e_->high(), e_->low());
            break;
        }
        case Expr::Kind::CONCAT: {
            auto e_ = std::static_pointer_cast<const ConcatExpr>(e);
            res     = "( ";
            res += to_string_inner(e_->els().at(0), cache);
            for (uint32_t i = 1; i < e_->els().size(); ++i) {
                res += " # ";
                res += to_string_inner(e_->els().at(i), cache);
            }
            res += " )";
            break;
        }
        case Expr::Kind::ZEXT: {
            auto e_ = std::static_pointer_cast<const ZextExpr>(e);
            res     = string_format("zext(%s, %lu)",
                                    to_string_inner(e_->expr(), cache).c_str(),
                                    e_->size());
            break;
        }
        case Expr::Kind::SEXT: {
            auto e_ = std::static_pointer_cast<const SextExpr>(e);
            res     = string_format("sext(%s, %lu)",
                                    to_string_inner(e_->expr(), cache).c_str(),
                                    e_->size());
            break;
        }
        case Expr::Kind::ITE: {
            auto e_ = std::static_pointer_cast<const ITEExpr>(e);
            res     = string_format("ITE(%s, %s, %s)",
                                    to_string_inner(e_->guard(), cache).c_str(),
                                    to_string_inner(e_->iftrue(), cache).c_str(),
                                    to_string_inner(e_->iffalse(), cache).c_str());
            break;
        }
        case Expr::Kind::SHL: {
            auto e_ = std::static_pointer_cast<const ShlExpr>(e);
            res     = string_format("( %s << %s )",
                                    to_string_inner(e_->expr(), cache).c_str(),
                                    to_string_inner(e_->val(), cache).c_str());
            break;
        }
        case Expr::Kind::LSHR: {
            auto e_ = std::static_pointer_cast<const LShrExpr>(e);
            res     = string_format("( %s l>> %s )",
                                    to_string_inner(e_->expr(), cache).c_str(),
                                    to_string_inner(e_->val(), cache).c_str());
            break;
        }
        case Expr::Kind::ASHR: {
            auto e_ = std::static_pointer_cast<const AShrExpr>(e);
            res     = string_format("( %s a>> %s )",
                                    to_string_inner(e_->expr(), cache).c_str(),
                                    to_string_inner(e_->val(), cache).c_str());
            break;
        }
        case Expr::Kind::NEG: {
            auto e_ = std::static_pointer_cast<const NegExpr>(e);
            res     = string_format("-%s", to_string_inner(e_->expr(), cache));
            break;
        }
        case Expr::Kind::NOT: {
            auto e_ = std::static_pointer_cast<const NotExpr>(e);
            res     = string_format("~%s", to_string_inner(e_->expr(), cache));
            break;
        }
        case Expr::Kind::AND: {
            auto e_ = std::static_pointer_cast<const AndExpr>(e);
            res     = "( ";
            res += to_string_inner(e_->els().at(0), cache);
            for (uint32_t i = 1; i < e_->els().size(); ++i) {
                res += " & ";
                res += to_string_inner(e_->els().at(i), cache);
            }
            res += " )";
            break;
        }
        case Expr::Kind::OR: {
            auto e_ = std::static_pointer_cast<const OrExpr>(e);
            res     = "( ";
            res += to_string_inner(e_->els().at(0), cache);
            for (uint32_t i = 1; i < e_->els().size(); ++i) {
                res += " | ";
                res += to_string_inner(e_->els().at(i), cache);
            }
            res += " )";
            break;
        }
        case Expr::Kind::XOR: {
            auto e_ = std::static_pointer_cast<const XorExpr>(e);
            res     = "( ";
            res += to_string_inner(e_->els().at(0), cache);
            for (uint32_t i = 1; i < e_->els().size(); ++i) {
                res += " ^ ";
                res += to_string_inner(e_->els().at(i), cache);
            }
            res += " )";
            break;
        }
        case Expr::Kind::ADD: {
            auto e_ = std::static_pointer_cast<const AddExpr>(e);
            res     = "( ";
            res += to_string_inner(e_->addends().at(0), cache);
            for (uint32_t i = 1; i < e_->addends().size(); ++i) {
                res += " + ";
                res += to_string_inner(e_->addends().at(i), cache);
            }
            res += " )";
            break;
        }
        case Expr::Kind::MUL: {
            auto e_ = std::static_pointer_cast<const MulExpr>(e);
            res     = "( ";
            res += to_string_inner(e_->els().at(0), cache);
            for (uint32_t i = 1; i < e_->els().size(); ++i) {
                res += " * ";
                res += to_string_inner(e_->els().at(i), cache);
            }
            res += " )";
            break;
        }
        case Expr::Kind::SDIV: {
            auto e_ = std::static_pointer_cast<const SDivExpr>(e);
            res     = string_format("( %s s/ %s )",
                                    to_string_inner(e_->lhs(), cache).c_str(),
                                    to_string_inner(e_->rhs(), cache).c_str());
            break;
        }
        case Expr::Kind::UDIV: {
            auto e_ = std::static_pointer_cast<const UDivExpr>(e);
            res     = string_format("( %s /u %s )",
                                    to_string_inner(e_->lhs(), cache).c_str(),
                                    to_string_inner(e_->rhs(), cache).c_str());
            break;
        }
        case Expr::Kind::SREM: {
            auto e_ = std::static_pointer_cast<const SRemExpr>(e);
            res     = string_format("( %s s%% %s )",
                                    to_string_inner(e_->lhs(), cache).c_str(),
                                    to_string_inner(e_->rhs(), cache).c_str());
            break;
        }
        case Expr::Kind::UREM: {
            auto e_ = std::static_pointer_cast<const URemExpr>(e);
            res     = string_format("( %s u%% %s )",
                                    to_string_inner(e_->lhs(), cache).c_str(),
                                    to_string_inner(e_->rhs(), cache).c_str());
            break;
        }
        case Expr::Kind::BOOL_NOT: {
            auto e_ = std::static_pointer_cast<const BoolNotExpr>(e);
            res     = string_format("!%s",
                                    to_string_inner(e_->expr(), cache).c_str());
            break;
        }
        case Expr::Kind::ULT: {
            auto e_ = std::static_pointer_cast<const UltExpr>(e);
            res     = string_format("( %s u< %s )",
                                    to_string_inner(e_->lhs(), cache).c_str(),
                                    to_string_inner(e_->rhs(), cache).c_str());
            break;
        }
        case Expr::Kind::ULE: {
            auto e_ = std::static_pointer_cast<const UleExpr>(e);
            res     = string_format("( %s u<= %s )",
                                    to_string_inner(e_->lhs(), cache).c_str(),
                                    to_string_inner(e_->rhs(), cache).c_str());
            break;
        }
        case Expr::Kind::UGT: {
            auto e_ = std::static_pointer_cast<const UgtExpr>(e);
            res     = string_format("( %s u> %s )",
                                    to_string_inner(e_->lhs(), cache).c_str(),
                                    to_string_inner(e_->rhs(), cache).c_str());
            break;
        }
        case Expr::Kind::UGE: {
            auto e_ = std::static_pointer_cast<const UgeExpr>(e);
            res     = string_format("( %s u>= %s )",
                                    to_string_inner(e_->lhs(), cache).c_str(),
                                    to_string_inner(e_->rhs(), cache).c_str());
            break;
        }
        case Expr::Kind::SLT: {
            auto e_ = std::static_pointer_cast<const SltExpr>(e);
            res     = string_format("( %s s< %s )",
                                    to_string_inner(e_->lhs(), cache).c_str(),
                                    to_string_inner(e_->rhs(), cache).c_str());
            break;
        }
        case Expr::Kind::SLE: {
            auto e_ = std::static_pointer_cast<const SleExpr>(e);
            res     = string_format("( %s s<= %s )",
                                    to_string_inner(e_->lhs(), cache).c_str(),
                                    to_string_inner(e_->rhs(), cache).c_str());
            break;
        }
        case Expr::Kind::SGT: {
            auto e_ = std::static_pointer_cast<const SgtExpr>(e);
            res     = string_format("( %s s> %s )",
                                    to_string_inner(e_->lhs(), cache).c_str(),
                                    to_string_inner(e_->rhs(), cache).c_str());
            break;
        }
        case Expr::Kind::SGE: {
            auto e_ = std::static_pointer_cast<const SgeExpr>(e);
            res     = string_format("( %s s>= %s )",
                                    to_string_inner(e_->lhs(), cache).c_str(),
                                    to_string_inner(e_->rhs(), cache).c_str());
            break;
        }
        case Expr::Kind::EQ: {
            auto e_ = std::static_pointer_cast<const EqExpr>(e);
            res     = string_format("( %s == %s )",
                                    to_string_inner(e_->lhs(), cache).c_str(),
                                    to_string_inner(e_->rhs(), cache).c_str());
            break;
        }
        case Expr::Kind::BOOL_AND: {
            auto e_ = std::static_pointer_cast<const BoolAndExpr>(e);
            res     = "( ";
            res += to_string_inner(e_->exprs().at(0), cache);
            for (uint32_t i = 1; i < e_->exprs().size(); ++i) {
                res += " && ";
                res += to_string_inner(e_->exprs().at(i), cache);
            }
            res += " )";
            break;
        }
        case Expr::Kind::BOOL_OR: {
            auto e_ = std::static_pointer_cast<const BoolOrExpr>(e);
            res     = "( ";
            res += to_string_inner(e_->exprs().at(0), cache);
            for (uint32_t i = 1; i < e_->exprs().size(); ++i) {
                res += " || ";
                res += to_string_inner(e_->exprs().at(i), cache);
            }
            res += " )";
            break;
        }
        default:
            err("expr::to_string_inner")
                << "unexpected kind " << e->kind() << std::endl;
            exit_fail();
    }

    cache.emplace(e, res);
    return res;
}

std::string expr_to_string(ExprPtr e)
{
    std::map<ExprPtr, std::string> cache;

    auto res = to_string_inner(e, cache);
    return res;
}

} // namespace naaz::expr
