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

    CheckResult check(const ConstraintManager& constraints,
                      expr::BoolExprPtr        query);

    std::map<std::string, expr::BVConst> model();
};

} // namespace naaz::solver
