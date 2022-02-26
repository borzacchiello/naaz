#pragma once

#include <memory>
#include <map>
#include <set>

#include "../expr/Expr.hpp"

namespace naaz::solver
{

class ConstraintManager
{
    // involved symbols cache
    std::map<expr::ExprPtr, std::set<std::string>> m_involved_symbols;

  public:
    ConstraintManager(const ConstraintManager& other);

    void              add_constraint(expr::BoolExprPtr constraint);
    expr::BoolExprPtr get_dependencies(expr::ExprPtr expr) const;

    static std::set<std::string> get_symbols(expr::ExprPtr expr);
};

} // namespace naaz::solver
