#include "../util/ioutil.hpp"

#include "util.hpp"
#include "ExprBuilder.hpp"

#define exprBuilder naaz::expr::ExprBuilder::The()

namespace naaz::expr
{

ExprPtr evaluate(ExprPtr e, const std::map<uint32_t, BVConst>& assignments,
                 bool model_completion)
{
    switch (e->kind()) {
        case Expr::Kind::SYM: {
            auto e_ = std::static_pointer_cast<const SymExpr>(e);
            if (assignments.contains(e_->id()))
                return exprBuilder.mk_const(assignments.at(e_->id()));

            if (model_completion)
                return exprBuilder.mk_const(0, e_->size());
            return e;
        }
        case Expr::Kind::CONST:
        case Expr::Kind::BOOL_CONST:
            return e;
        case Expr::Kind::EXTRACT: {
            auto e_     = std::static_pointer_cast<const ExtractExpr>(e);
            auto eval_e = std::static_pointer_cast<const BVExpr>(
                evaluate(e_->expr(), assignments, model_completion));
            return exprBuilder.mk_extract(eval_e, e_->high(), e_->low());
        }
        case Expr::Kind::CONCAT: {
            auto e_       = std::static_pointer_cast<const ConcatExpr>(e);
            auto eval_lhs = std::static_pointer_cast<const BVExpr>(
                evaluate(e_->lhs(), assignments, model_completion));
            auto eval_rhs = std::static_pointer_cast<const BVExpr>(
                evaluate(e_->rhs(), assignments, model_completion));
            return exprBuilder.mk_concat(eval_lhs, eval_rhs);
        }
        case Expr::Kind::ZEXT: {
            auto e_     = std::static_pointer_cast<const ZextExpr>(e);
            auto eval_e = std::static_pointer_cast<const BVExpr>(
                evaluate(e_->expr(), assignments, model_completion));
            return exprBuilder.mk_zext(eval_e, e_->size());
        }
        case Expr::Kind::SEXT: {
            auto e_     = std::static_pointer_cast<const SextExpr>(e);
            auto eval_e = std::static_pointer_cast<const BVExpr>(
                evaluate(e_, assignments, model_completion));
            return exprBuilder.mk_sext(eval_e, e_->size());
        }
        case Expr::Kind::ITE: {
            auto e_         = std::static_pointer_cast<const ITEExpr>(e);
            auto eval_guard = std::static_pointer_cast<const BoolExpr>(
                evaluate(e_->guard(), assignments, model_completion));
            auto eval_iftrue = std::static_pointer_cast<const BVExpr>(
                evaluate(e_->iftrue(), assignments, model_completion));
            auto eval_iffalse = std::static_pointer_cast<const BVExpr>(
                evaluate(e_->iffalse(), assignments, model_completion));
            return exprBuilder.mk_ite(eval_guard, eval_iftrue, eval_iffalse);
        }
        case Expr::Kind::SHL: {
            auto e_        = std::static_pointer_cast<const ShlExpr>(e);
            auto eval_expr = std::static_pointer_cast<const BVExpr>(
                evaluate(e_->expr(), assignments, model_completion));
            auto eval_val = std::static_pointer_cast<const BVExpr>(
                evaluate(e_->val(), assignments, model_completion));
            return exprBuilder.mk_shl(eval_expr, eval_val);
        }
        case Expr::Kind::LSHR: {
            auto e_        = std::static_pointer_cast<const LShrExpr>(e);
            auto eval_expr = std::static_pointer_cast<const BVExpr>(
                evaluate(e_->expr(), assignments, model_completion));
            auto eval_val = std::static_pointer_cast<const BVExpr>(
                evaluate(e_->val(), assignments, model_completion));
            return exprBuilder.mk_lshr(eval_expr, eval_val);
        }
        case Expr::Kind::ASHR: {
            auto e_        = std::static_pointer_cast<const AShrExpr>(e);
            auto eval_expr = std::static_pointer_cast<const BVExpr>(
                evaluate(e_->expr(), assignments, model_completion));
            auto eval_val = std::static_pointer_cast<const BVExpr>(
                evaluate(e_->val(), assignments, model_completion));
            return exprBuilder.mk_ashr(eval_expr, eval_val);
        }
        case Expr::Kind::NEG: {
            auto e_        = std::static_pointer_cast<const NegExpr>(e);
            auto eval_expr = std::static_pointer_cast<const BVExpr>(
                evaluate(e_->expr(), assignments, model_completion));
            return exprBuilder.mk_neg(eval_expr);
        }
        case Expr::Kind::AND: {
            auto e_        = std::static_pointer_cast<const AndExpr>(e);
            auto eval_expr = std::static_pointer_cast<const BVExpr>(
                evaluate(e_->els().at(0), assignments, model_completion));
            for (uint32_t i = 1; i < e_->els().size(); ++i)
                eval_expr = exprBuilder.mk_and(
                    eval_expr,
                    std::static_pointer_cast<const BVExpr>(evaluate(
                        e_->els().at(i), assignments, model_completion)));
            return eval_expr;
        }
        case Expr::Kind::OR: {
            auto e_        = std::static_pointer_cast<const OrExpr>(e);
            auto eval_expr = std::static_pointer_cast<const BVExpr>(
                evaluate(e_->els().at(0), assignments, model_completion));
            for (uint32_t i = 1; i < e_->els().size(); ++i)
                eval_expr = exprBuilder.mk_or(
                    eval_expr,
                    std::static_pointer_cast<const BVExpr>(evaluate(
                        e_->els().at(i), assignments, model_completion)));
            return eval_expr;
        }
        case Expr::Kind::XOR: {
            auto e_        = std::static_pointer_cast<const XorExpr>(e);
            auto eval_expr = std::static_pointer_cast<const BVExpr>(
                evaluate(e_->els().at(0), assignments, model_completion));
            for (uint32_t i = 1; i < e_->els().size(); ++i)
                eval_expr = exprBuilder.mk_xor(
                    eval_expr,
                    std::static_pointer_cast<const BVExpr>(evaluate(
                        e_->els().at(i), assignments, model_completion)));
            return eval_expr;
        }
        case Expr::Kind::ADD: {
            auto e_        = std::static_pointer_cast<const AddExpr>(e);
            auto eval_expr = std::static_pointer_cast<const BVExpr>(
                evaluate(e_->addends().at(0), assignments, model_completion));
            for (uint32_t i = 1; i < e_->addends().size(); ++i)
                eval_expr = exprBuilder.mk_add(
                    eval_expr,
                    std::static_pointer_cast<const BVExpr>(evaluate(
                        e_->addends().at(i), assignments, model_completion)));
            return eval_expr;
        }
        case Expr::Kind::NOT: {
            auto e_        = std::static_pointer_cast<const NotExpr>(e);
            auto eval_expr = std::static_pointer_cast<const BoolExpr>(
                evaluate(e_->expr(), assignments, model_completion));
            return exprBuilder.mk_not(eval_expr);
        }
        case Expr::Kind::ULT: {
            auto e_       = std::static_pointer_cast<const UltExpr>(e);
            auto eval_lhs = std::static_pointer_cast<const BVExpr>(
                evaluate(e_->lhs(), assignments, model_completion));
            auto eval_rhs = std::static_pointer_cast<const BVExpr>(
                evaluate(e_->rhs(), assignments, model_completion));
            return exprBuilder.mk_ult(eval_lhs, eval_rhs);
        }
        case Expr::Kind::ULE: {
            auto e_       = std::static_pointer_cast<const UleExpr>(e);
            auto eval_lhs = std::static_pointer_cast<const BVExpr>(
                evaluate(e_->lhs(), assignments, model_completion));
            auto eval_rhs = std::static_pointer_cast<const BVExpr>(
                evaluate(e_->rhs(), assignments, model_completion));
            return exprBuilder.mk_ule(eval_lhs, eval_rhs);
        }
        case Expr::Kind::UGT: {
            auto e_       = std::static_pointer_cast<const UgtExpr>(e);
            auto eval_lhs = std::static_pointer_cast<const BVExpr>(
                evaluate(e_->lhs(), assignments, model_completion));
            auto eval_rhs = std::static_pointer_cast<const BVExpr>(
                evaluate(e_->rhs(), assignments, model_completion));
            return exprBuilder.mk_ugt(eval_lhs, eval_rhs);
        }
        case Expr::Kind::UGE: {
            auto e_       = std::static_pointer_cast<const UgeExpr>(e);
            auto eval_lhs = std::static_pointer_cast<const BVExpr>(
                evaluate(e_->lhs(), assignments, model_completion));
            auto eval_rhs = std::static_pointer_cast<const BVExpr>(
                evaluate(e_->rhs(), assignments, model_completion));
            return exprBuilder.mk_uge(eval_lhs, eval_rhs);
        }
        case Expr::Kind::SLT: {
            auto e_       = std::static_pointer_cast<const SltExpr>(e);
            auto eval_lhs = std::static_pointer_cast<const BVExpr>(
                evaluate(e_->lhs(), assignments, model_completion));
            auto eval_rhs = std::static_pointer_cast<const BVExpr>(
                evaluate(e_->rhs(), assignments, model_completion));
            return exprBuilder.mk_slt(eval_lhs, eval_rhs);
        }
        case Expr::Kind::SLE: {
            auto e_       = std::static_pointer_cast<const SleExpr>(e);
            auto eval_lhs = std::static_pointer_cast<const BVExpr>(
                evaluate(e_->lhs(), assignments, model_completion));
            auto eval_rhs = std::static_pointer_cast<const BVExpr>(
                evaluate(e_->rhs(), assignments, model_completion));
            return exprBuilder.mk_sle(eval_lhs, eval_rhs);
        }
        case Expr::Kind::SGT: {
            auto e_       = std::static_pointer_cast<const SgtExpr>(e);
            auto eval_lhs = std::static_pointer_cast<const BVExpr>(
                evaluate(e_->lhs(), assignments, model_completion));
            auto eval_rhs = std::static_pointer_cast<const BVExpr>(
                evaluate(e_->rhs(), assignments, model_completion));
            return exprBuilder.mk_sgt(eval_lhs, eval_rhs);
        }
        case Expr::Kind::SGE: {
            auto e_       = std::static_pointer_cast<const SgeExpr>(e);
            auto eval_lhs = std::static_pointer_cast<const BVExpr>(
                evaluate(e_->lhs(), assignments, model_completion));
            auto eval_rhs = std::static_pointer_cast<const BVExpr>(
                evaluate(e_->rhs(), assignments, model_completion));
            return exprBuilder.mk_sge(eval_lhs, eval_rhs);
        }
        case Expr::Kind::EQ: {
            auto e_       = std::static_pointer_cast<const EqExpr>(e);
            auto eval_lhs = std::static_pointer_cast<const BVExpr>(
                evaluate(e_->lhs(), assignments, model_completion));
            auto eval_rhs = std::static_pointer_cast<const BVExpr>(
                evaluate(e_->rhs(), assignments, model_completion));
            return exprBuilder.mk_eq(eval_lhs, eval_rhs);
        }
        case Expr::Kind::BOOL_AND: {
            auto e_        = std::static_pointer_cast<const AddExpr>(e);
            auto eval_expr = std::static_pointer_cast<const BoolExpr>(
                evaluate(e_->addends().at(0), assignments, model_completion));
            for (uint32_t i = 1; i < e_->addends().size(); ++i)
                eval_expr = exprBuilder.mk_bool_and(
                    eval_expr,
                    std::static_pointer_cast<const BoolExpr>(evaluate(
                        e_->addends().at(i), assignments, model_completion)));
            return eval_expr;
        }
        default:
            break;
    }

    err("expr::evaluate") << "unexpected kind " << e->kind() << std::endl;
    exit_fail();
}

} // namespace naaz::expr
