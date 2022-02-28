#include "Z3Solver.hpp"

namespace naaz::solver
{

Z3Solver::Z3Solver() : m_solver(m_ctx) {}

CheckResult Z3Solver::check(const ConstraintManager& constraints,
                            expr::BoolExprPtr        query)
{
    expr::ExprPtr pi = constraints.build_query(query);

    m_solver.reset();
    m_solver.add(pi->to_z3(m_ctx));
    m_solver.add(query->to_z3(m_ctx));

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

std::map<std::string, expr::BVConst> Z3Solver::model()
{
    std::map<std::string, expr::BVConst> res;

    z3::model model = m_solver.get_model();
    for (uint32_t i = 0; i < model.size(); i++) {
        z3::func_decl v = model[i];
        assert(v.arity() == 0);

        z3::expr val = model.get_const_interp(v);

        expr::BVConst bv(val.get_decimal_string(1),
                         (ssize_t)val.get_sort().bv_size());
        res.emplace(v.name().str(), bv);
    }
    return res;
}

} // namespace naaz::solver
