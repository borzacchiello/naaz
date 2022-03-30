#include "../expr/ExprBuilder.hpp"
#include "../expr/util.hpp"
#include "../util/ioutil.hpp"

#include "Z3Solver.hpp"

#define exprBuilder naaz::expr::ExprBuilder::The()

namespace naaz::solver
{

Z3Solver::Z3Solver() : m_solver(m_ctx) {}

CheckResult Z3Solver::check(expr::BoolExprPtr query)
{
    m_solver.reset();
    m_solver.add(to_z3(query));

    CheckResult res = CheckResult::UNKNOWN;
    switch (m_solver.check()) {
        case z3::unsat:
            res = CheckResult::UNSAT;
            break;
        case z3::sat:
            res = CheckResult::SAT;
            break;
    }

    return res;
}

std::vector<expr::BVConst> Z3Solver::eval_upto(expr::BVExprPtr   val,
                                               expr::BoolExprPtr pi, int32_t n)
{
    std::vector<expr::BVConst> res;

    auto val_z3 = to_z3(val);
    auto pi_z3  = to_z3(pi);

    m_solver.reset();
    m_solver.add(pi_z3);
    while (n-- > 0) {
        auto r = m_solver.check();
        if (r != z3::sat)
            break;

        auto m        = model();
        auto val_conc = std::static_pointer_cast<const expr::ConstExpr>(
            expr::evaluate(val, m, true));
        res.push_back(val_conc->val());
        m_solver.add(to_z3(val_conc) != val_z3);
    }

    return res;
}

std::map<uint32_t, expr::BVConst> Z3Solver::model()
{
    std::map<uint32_t, expr::BVConst> res;

    z3::model model = m_solver.get_model();
    for (uint32_t i = 0; i < model.size(); i++) {
        z3::func_decl v = model[i];
        assert(v.arity() == 0);

        z3::expr val = model.get_const_interp(v);

        expr::BVConst bv(val.get_decimal_string(1),
                         (ssize_t)val.get_sort().bv_size());
        res.emplace(exprBuilder.get_sym_id(v.name().str()), bv);
    }
    return res;
}

static uint32_t countSetBits(uint64_t n)
{
    uint32_t count = 0;
    while (n) {
        count += n & 1;
        n >>= 1;
    }
    return count;
}

static z3::sort get_fp_sort(z3::context& ctx, FloatFormatPtr ff)
{
    constexpr uint64_t mask = (2UL << (63UL)) - 1UL;
    uint32_t fract_size     = countSetBits(ff->extractFractionalCode(mask)) + 1;
    uint32_t exp_size       = ff->getSize() * 8UL - fract_size;

    return ctx.fpa_sort(exp_size, fract_size);
}

static z3::expr to_z3_inner(z3::context& ctx, expr::ExprPtr e,
                            std::map<expr::ExprPtr, z3::expr>& cache)
{
    if (cache.contains(e))
        return cache.at(e);

    z3::expr res(ctx);
    switch (e->kind()) {
        case expr::Expr::Kind::SYM: {
            auto e_ = std::static_pointer_cast<const expr::SymExpr>(e);
            res     = ctx.bv_const(e_->name().c_str(), e_->size());
            break;
        }
        case expr::Expr::Kind::CONST: {
            auto e_ = std::static_pointer_cast<const expr::ConstExpr>(e);
            res     = ctx.bv_val(e_->val().to_string().c_str(), e_->size());
            break;
        }
        case expr::Expr::Kind::BOOL_CONST: {
            auto e_ = std::static_pointer_cast<const expr::BoolConst>(e);
            if (e_->is_true())
                res = ctx.bool_val(true);
            else
                res = ctx.bool_val(false);
            break;
        }
        case expr::Expr::Kind::EXTRACT: {
            auto e_ = std::static_pointer_cast<const expr::ExtractExpr>(e);
            res     = to_z3_inner(ctx, e_->expr(), cache)
                      .extract(e_->high(), e_->low());
            break;
        }
        case expr::Expr::Kind::CONCAT: {
            auto e_ = std::static_pointer_cast<const expr::ConcatExpr>(e);
            res     = to_z3_inner(ctx, e_->els().at(0), cache);
            for (uint64_t i = 1; i < e_->els().size(); ++i)
                res = z3::concat(res, to_z3_inner(ctx, e_->els().at(i), cache));
            break;
        }
        case expr::Expr::Kind::ZEXT: {
            auto e_ = std::static_pointer_cast<const expr::ZextExpr>(e);
            res     = z3::zext(to_z3_inner(ctx, e_->expr(), cache),
                               e_->size() - e_->expr()->size());
            break;
        }
        case expr::Expr::Kind::SEXT: {
            auto e_ = std::static_pointer_cast<const expr::SextExpr>(e);
            res     = z3::sext(to_z3_inner(ctx, e_->expr(), cache),
                               e_->size() - e_->expr()->size());
            break;
        }
        case expr::Expr::Kind::ITE: {
            auto e_ = std::static_pointer_cast<const expr::ITEExpr>(e);
            res     = z3::ite(to_z3_inner(ctx, e_->guard(), cache),
                              to_z3_inner(ctx, e_->iftrue(), cache),
                              to_z3_inner(ctx, e_->iffalse(), cache));
            break;
        }
        case expr::Expr::Kind::SHL: {
            auto e_ = std::static_pointer_cast<const expr::ShlExpr>(e);
            res     = z3::shl(to_z3_inner(ctx, e_->expr(), cache),
                              to_z3_inner(ctx, e_->val(), cache));
            break;
        }
        case expr::Expr::Kind::LSHR: {
            auto e_ = std::static_pointer_cast<const expr::LShrExpr>(e);
            res     = z3::lshr(to_z3_inner(ctx, e_->expr(), cache),
                               to_z3_inner(ctx, e_->val(), cache));
            break;
        }
        case expr::Expr::Kind::ASHR: {
            auto e_ = std::static_pointer_cast<const expr::AShrExpr>(e);
            res     = z3::ashr(to_z3_inner(ctx, e_->expr(), cache),
                               to_z3_inner(ctx, e_->val(), cache));
            break;
        }
        case expr::Expr::Kind::NEG: {
            auto e_ = std::static_pointer_cast<const expr::NegExpr>(e);
            res     = -to_z3_inner(ctx, e_->expr(), cache);
            break;
        }
        case expr::Expr::Kind::NOT: {
            auto e_ = std::static_pointer_cast<const expr::NotExpr>(e);
            res     = ~to_z3_inner(ctx, e_->expr(), cache);
            break;
        }
        case expr::Expr::Kind::AND: {
            auto e_ = std::static_pointer_cast<const expr::AndExpr>(e);
            res     = to_z3_inner(ctx, e_->els().at(0), cache);
            for (uint64_t i = 1; i < e_->els().size(); ++i)
                res = res & to_z3_inner(ctx, e_->els().at(i), cache);
            break;
        }
        case expr::Expr::Kind::OR: {
            auto e_ = std::static_pointer_cast<const expr::OrExpr>(e);
            res     = to_z3_inner(ctx, e_->els().at(0), cache);
            for (uint64_t i = 1; i < e_->els().size(); ++i)
                res = res | to_z3_inner(ctx, e_->els().at(i), cache);
            break;
        }
        case expr::Expr::Kind::XOR: {
            auto e_ = std::static_pointer_cast<const expr::XorExpr>(e);
            res     = to_z3_inner(ctx, e_->els().at(0), cache);
            for (uint64_t i = 1; i < e_->els().size(); ++i)
                res = res ^ to_z3_inner(ctx, e_->els().at(i), cache);
            break;
        }
        case expr::Expr::Kind::ADD: {
            auto e_ = std::static_pointer_cast<const expr::AddExpr>(e);
            res     = to_z3_inner(ctx, e_->addends().at(0), cache);
            for (uint64_t i = 1; i < e_->addends().size(); ++i)
                res = res + to_z3_inner(ctx, e_->addends().at(i), cache);
            break;
        }
        case expr::Expr::Kind::MUL: {
            auto e_ = std::static_pointer_cast<const expr::MulExpr>(e);
            res     = to_z3_inner(ctx, e_->els().at(0), cache);
            for (uint64_t i = 1; i < e_->els().size(); ++i)
                res = res * to_z3_inner(ctx, e_->els().at(i), cache);
            break;
        }
        case expr::Expr::Kind::SDIV: {
            auto e_ = std::static_pointer_cast<const expr::SDivExpr>(e);
            res     = to_z3_inner(ctx, e_->lhs(), cache) /
                  to_z3_inner(ctx, e_->rhs(), cache);
            break;
        }
        case expr::Expr::Kind::UDIV: {
            auto e_ = std::static_pointer_cast<const expr::UDivExpr>(e);
            res     = z3::udiv(to_z3_inner(ctx, e_->lhs(), cache),
                               to_z3_inner(ctx, e_->rhs(), cache));
            break;
        }
        case expr::Expr::Kind::SREM: {
            auto e_ = std::static_pointer_cast<const expr::SRemExpr>(e);
            res     = to_z3_inner(ctx, e_->lhs(), cache) %
                  to_z3_inner(ctx, e_->rhs(), cache);
            break;
        }
        case expr::Expr::Kind::UREM: {
            auto e_ = std::static_pointer_cast<const expr::URemExpr>(e);
            res     = z3::urem(to_z3_inner(ctx, e_->lhs(), cache),
                               to_z3_inner(ctx, e_->rhs(), cache));
            break;
        }
        case expr::Expr::Kind::BOOL_NOT: {
            auto e_ = std::static_pointer_cast<const expr::BoolNotExpr>(e);
            res     = !to_z3_inner(ctx, e_->expr(), cache);
            break;
        }
        case expr::Expr::Kind::ULT: {
            auto e_ = std::static_pointer_cast<const expr::UltExpr>(e);
            res     = z3::ult(to_z3_inner(ctx, e_->lhs(), cache),
                              to_z3_inner(ctx, e_->rhs(), cache));
            break;
        }
        case expr::Expr::Kind::ULE: {
            auto e_ = std::static_pointer_cast<const expr::UleExpr>(e);
            res     = z3::ule(to_z3_inner(ctx, e_->lhs(), cache),
                              to_z3_inner(ctx, e_->rhs(), cache));
            break;
        }
        case expr::Expr::Kind::UGT: {
            auto e_ = std::static_pointer_cast<const expr::UgtExpr>(e);
            res     = z3::ugt(to_z3_inner(ctx, e_->lhs(), cache),
                              to_z3_inner(ctx, e_->rhs(), cache));
            break;
        }
        case expr::Expr::Kind::UGE: {
            auto e_ = std::static_pointer_cast<const expr::UgeExpr>(e);
            res     = z3::uge(to_z3_inner(ctx, e_->lhs(), cache),
                              to_z3_inner(ctx, e_->rhs(), cache));
            break;
        }
        case expr::Expr::Kind::SLT: {
            auto e_ = std::static_pointer_cast<const expr::SltExpr>(e);
            res     = z3::slt(to_z3_inner(ctx, e_->lhs(), cache),
                              to_z3_inner(ctx, e_->rhs(), cache));
            break;
        }
        case expr::Expr::Kind::SLE: {
            auto e_ = std::static_pointer_cast<const expr::SleExpr>(e);
            res     = z3::sle(to_z3_inner(ctx, e_->lhs(), cache),
                              to_z3_inner(ctx, e_->rhs(), cache));
            break;
        }
        case expr::Expr::Kind::SGT: {
            auto e_ = std::static_pointer_cast<const expr::SgtExpr>(e);
            res     = z3::sgt(to_z3_inner(ctx, e_->lhs(), cache),
                              to_z3_inner(ctx, e_->rhs(), cache));
            break;
        }
        case expr::Expr::Kind::SGE: {
            auto e_ = std::static_pointer_cast<const expr::SgeExpr>(e);
            res     = z3::sge(to_z3_inner(ctx, e_->lhs(), cache),
                              to_z3_inner(ctx, e_->rhs(), cache));
            break;
        }
        case expr::Expr::Kind::EQ: {
            auto e_ = std::static_pointer_cast<const expr::EqExpr>(e);
            res     = to_z3_inner(ctx, e_->lhs(), cache) ==
                  to_z3_inner(ctx, e_->rhs(), cache);
            break;
        }
        case expr::Expr::Kind::BOOL_AND: {
            auto e_ = std::static_pointer_cast<const expr::BoolAndExpr>(e);
            res     = to_z3_inner(ctx, e_->exprs().at(0), cache);
            for (uint64_t i = 1; i < e_->exprs().size(); ++i)
                res = res && to_z3_inner(ctx, e_->exprs().at(i), cache);
            break;
        }
        case expr::Expr::Kind::BOOL_OR: {
            auto e_ = std::static_pointer_cast<const expr::BoolOrExpr>(e);
            res     = to_z3_inner(ctx, e_->exprs().at(0), cache);
            for (uint64_t i = 1; i < e_->exprs().size(); ++i)
                res = res || to_z3_inner(ctx, e_->exprs().at(i), cache);
            break;
        }
        case expr::Expr::Kind::FP_CONST: {
            auto e_ = std::static_pointer_cast<const expr::FPConstExpr>(e);
            res     = ctx.bv_val(e_->val().val(), e_->val().size());
            res     = res.mk_from_ieee_bv(get_fp_sort(ctx, e_->ff()));
            break;
        }
        case expr::Expr::Kind::BV_TO_FP: {
            auto e_ = std::static_pointer_cast<const expr::BVToFPExpr>(e);
            res     = to_z3_inner(ctx, e_->expr(), cache);
            res     = res.mk_from_ieee_bv(get_fp_sort(ctx, e_->ff()));
            break;
        }
        case expr::Expr::Kind::FP_TO_BV: {
            auto e_ = std::static_pointer_cast<const expr::FPToBVExpr>(e);
            res     = to_z3_inner(ctx, e_->expr(), cache).mk_to_ieee_bv();
            break;
        }
        case expr::Expr::Kind::FP_CONVERT: {
            auto e_ = std::static_pointer_cast<const expr::FPConvert>(e);
            res     = z3::fpa_to_fpa(to_z3_inner(ctx, e_->expr(), cache),
                                     get_fp_sort(ctx, e_->ff()));
            break;
        }
        case expr::Expr::Kind::FP_INT_TO_FP: {
            auto e_ = std::static_pointer_cast<const expr::IntToFPExpr>(e);
            res     = z3::sbv_to_fpa(to_z3_inner(ctx, e_->expr(), cache),
                                     get_fp_sort(ctx, e_->ff()));
            break;
        }
        case expr::Expr::Kind::FP_IS_NAN: {
            auto e_ = std::static_pointer_cast<const expr::FPIsNAN>(e);
            res     = to_z3_inner(ctx, e_->expr(), cache) ==
                  ctx.fpa_nan(get_fp_sort(ctx, e_->expr()->ff()));
            break;
        }
        case expr::Expr::Kind::FP_DIV: {
            auto e_ = std::static_pointer_cast<const expr::FPDivExpr>(e);
            res     = to_z3_inner(ctx, e_->lhs(), cache) /
                  to_z3_inner(ctx, e_->rhs(), cache);
            break;
        }
        case expr::Expr::Kind::FP_EQ: {
            auto e_ = std::static_pointer_cast<const expr::FPEqExpr>(e);
            res     = to_z3_inner(ctx, e_->lhs(), cache) ==
                  to_z3_inner(ctx, e_->rhs(), cache);
            break;
        }
        case expr::Expr::Kind::FP_LT: {
            auto e_ = std::static_pointer_cast<const expr::FPLtExpr>(e);
            res     = to_z3_inner(ctx, e_->lhs(), cache) <
                  to_z3_inner(ctx, e_->rhs(), cache);
            break;
        }
        default:
            err("expr::to_z3_inner")
                << "unexpected kind " << e->kind() << std::endl;
            exit_fail();
    }

    cache.emplace(e, res);
    return res;
}

z3::expr Z3Solver::to_z3(expr::ExprPtr e)
{
    std::map<expr::ExprPtr, z3::expr> cache;

    auto res = to_z3_inner(m_ctx, e, cache);
    return res;
}

} // namespace naaz::solver
