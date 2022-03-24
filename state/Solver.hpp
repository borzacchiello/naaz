#pragma once

#include "../expr/Expr.hpp"
#include "../solver/ConstraintManager.hpp"
#include "../solver/Z3Solver.hpp"

namespace naaz::state
{

class Solver
{
    solver::ConstraintManager         m_manager;
    std::map<uint32_t, expr::BVConst> m_model;

  public:
    Solver() {}
    Solver(const Solver& other)
        : m_manager(other.m_manager), m_model(other.m_model)
    {
    }

    const solver::ConstraintManager& manager() const { return m_manager; }

    void                add(expr::BoolExprPtr c);
    solver::CheckResult check_sat(expr::BoolExprPtr c,
                                  bool              populate_model = true);

    expr::BVConst              evaluate(expr::ExprPtr e);
    std::vector<expr::BVConst> evaluate_upto(expr::BVExprPtr e, int n);
};

} // namespace naaz::state
