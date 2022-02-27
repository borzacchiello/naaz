#pragma once

#include <memory>
#include <map>
#include <set>

#include "../expr/Expr.hpp"

namespace naaz::solver
{

class ConstraintManager
{
    // involved symbols cache. Shared among all constraint managers!
    static std::map<expr::ExprPtr, std::set<uint32_t>> g_involved_symbols;

    static const std::set<uint32_t>&
    get_involved_inputs(expr::ExprPtr constraint);

    // constraints saved as mappings between symbols (strings) and expressions
    std::map<uint32_t, std::set<expr::BoolExprPtr>> m_constraints;

    // symbol depencency graph
    std::map<uint32_t, std::set<uint32_t>> m_dependencies;

    std::set<uint32_t> get_dependencies(expr::BoolExprPtr constraint) const;

  public:
    ConstraintManager() {}
    ConstraintManager(const ConstraintManager& other);
    ~ConstraintManager() {}

    void              add(expr::BoolExprPtr constraint);
    expr::BoolExprPtr build_query(expr::BoolExprPtr constraint) const;
};

} // namespace naaz::solver
