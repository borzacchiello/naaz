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

    CheckResult check(expr::BoolExprPtr query);

    std::map<uint32_t, expr::BVConst> model();
};

} // namespace naaz::solver
