#pragma once

#include <optional>

#include "../expr/Expr.hpp"
#include "../solver/ConstraintManager.hpp"
#include "../solver/Z3Solver.hpp"

namespace naaz::state
{

class Solver
{
    solver::ConstraintManager         m_manager;
    std::map<uint32_t, expr::BVConst> m_model;

    solver::CheckResult check_sat(expr::BoolExprPtr c,
                                  bool              populate_model = true);

  public:
    Solver() {}
    Solver(const Solver& other)
        : m_manager(other.m_manager), m_model(other.m_model)
    {
    }

    const solver::ConstraintManager& manager() const { return m_manager; }
    solver::CheckResult              satisfiable();

    void                add(expr::BoolExprPtr c);
    solver::CheckResult check_sat_and_add_if_sat(expr::BoolExprPtr c);
    solver::CheckResult may_be_true(expr::BoolExprPtr c);

    // due to lazy constraints, PI could be unsat, and the evaluation can fail
    std::optional<expr::BVConst>              evaluate(expr::ExprPtr e);
    std::optional<std::vector<expr::BVConst>> evaluate_upto(expr::BVExprPtr e,
                                                            int             n);
};

} // namespace naaz::state
