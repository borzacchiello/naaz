#include <cassert>

#include "Solver.hpp"
#include "../expr/util.hpp"
#include "../expr/ExprBuilder.hpp"
#include "../util/ioutil.hpp"

#define exprBuilder naaz::expr::ExprBuilder::The()

namespace naaz::state
{

void Solver::add(expr::BoolExprPtr c) { m_manager.add(c); }

solver::CheckResult Solver::check_sat(expr::BoolExprPtr c, bool populate_model)
{
    auto expr_in_current_model = expr::evaluate(c, m_model, false);
    if (expr_in_current_model->kind() == expr::Expr::Kind::BOOL_CONST) {
        auto e_ = std::static_pointer_cast<const expr::BoolConst>(
            expr_in_current_model);
        if (e_->is_true())
            return solver::CheckResult::SAT;
    }
    assert(expr_in_current_model->kind() != expr::Expr::Kind::CONST &&
           "Solver::check_sat(): unexpected expr::evaluate result");

    solver::CheckResult res = solver::Z3Solver::The().check(
        exprBuilder.mk_bool_and(m_manager.pi(c), c));
    if (res == solver::CheckResult::SAT && populate_model) {
        std::map<uint32_t, expr::BVConst> model =
            solver::Z3Solver::The().model();
        for (const auto& [sym, val] : model)
            m_model[sym] = val;
    }

    return res;
}

expr::BVConst Solver::evaluate(expr::ExprPtr e)
{
    std::set<uint32_t> involved_symbols = m_manager.get_dependencies(e);
    for (auto s_id : involved_symbols) {
        if (!m_model.contains(s_id)) {
            solver::CheckResult r = check_sat(m_manager.pi(e));
            if (r != solver::CheckResult::SAT) {
                err("state::Solver") << "evaluate(): PI is UNSAT" << std::endl;
                exit_fail();
            }
            break;
        }
    }

    auto eval = expr::evaluate(e, m_model, true);
    if (eval->kind() == expr::Expr::Kind::CONST)
        return std::static_pointer_cast<const expr::ConstExpr>(eval)->val();
    assert(eval->kind() == expr::Expr::Kind::BOOL_CONST &&
           "Solver::evaluate(): unexpected expr::evaluate result");
    return std::static_pointer_cast<const expr::BoolConst>(eval)->is_true()
               ? expr::BVConst(1UL, 1)
               : expr::BVConst(0UL, 1);
}

} // namespace naaz::state
