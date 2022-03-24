#pragma once

#include "ConstraintManager.hpp"
#include "z3++.h"

namespace naaz::solver
{

enum CheckResult { SAT, UNSAT, UNKNOWN };

class Z3Solver
{
    z3::context m_ctx;
    z3::solver  m_solver;

    Z3Solver();

  public:
    static Z3Solver& The()
    {
        static Z3Solver solv;
        return solv;
    }

    z3::expr to_z3(expr::ExprPtr e);

    CheckResult                check(expr::BoolExprPtr query);
    std::vector<expr::BVConst> eval_upto(expr::BVExprPtr   val,
                                         expr::BoolExprPtr pi, int32_t n);

    std::map<uint32_t, expr::BVConst> model();
};

} // namespace naaz::solver
